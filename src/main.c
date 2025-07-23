#include <unistd.h> // for isatty()
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "parse.h"
#include "sema.h"
#include "compile.h" 
#include "vm.h"
#include "debug.h"

int main(int argc, const char **argv) 
{
    if (argc != 2) {
        printf("Usage: ./build/flood [script]\n");
        return 1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("File `%s` does not exist\n", argv[1]);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp); 
    fseek(fp, 0, SEEK_SET);
    char *buf = allocate(length+2);
    buf[0] = '\0';
    buf[length+1] = '\0';
    char *source = buf+1;
    // TODO check for null bytes
    fread(source, 1, length, fp);
    bool color = isatty(1);

    struct Parser parser;
    init_parser(&parser);
    struct FnDeclNode *ast = parse(&parser, source);
    if (parser.errlist.cnt > 0) {
        print_errlist(&parser.errlist, color);
        goto err_release_parser;
    }

    // sym_arr is shared by the sema and compiler
    struct SymArr sym_arr;
    init_symbol_arr(&sym_arr);

    struct SemaState sema;
    init_sema_state(&sema, &sym_arr);
    analyze(&sema, ast);
    if (sema.errlist.cnt > 0) {
        print_errlist(&sema.errlist, color);
        release_symbol_arr(&sym_arr);
        goto err_release_sema_state;
    }

    // DELETEME!
    print_node(ast, 0);
    return 0;

    struct Compiler compiler;
    init_compiler(&compiler, &sym_arr);
    struct VM vm;
    init_vm(&vm);

    struct ClosureObj *closure = compile_file(&vm, &compiler, ast);
    if (compiler.errlist.cnt > 0) {
        print_errlist(&compiler.errlist, color);
        release_symbol_arr(&sym_arr);
        goto err_release_compiler;
    }

    // invoke main function if defined
    if (compiler.main_fn_idx != -1) {
        emit_byte(&closure->fn->chunk, OP_GET_GLOBAL, 1);
        emit_byte(&closure->fn->chunk, compiler.main_fn_idx, 1);
        emit_byte(&closure->fn->chunk, OP_CALL, 1);
        emit_byte(&closure->fn->chunk, 0, 1);
    }
    emit_byte(&closure->fn->chunk, OP_NIL, 1);
    emit_byte(&closure->fn->chunk, OP_RETURN, 1);
    // push globals
    for (i32 i = 0; i < compiler.fn_node->body->cnt; i++)
        push_val_array(&vm.globals, MK_NIL);

    release_symbol_arr(&sym_arr);
    release_compiler(&compiler);
    release_sema_state(&sema);
    release_parser(&parser);
    release(buf);
    fclose(fp);

    run_vm(&vm, closure);
    release_vm(&vm);
    return 0;

err_release_compiler:
    release_compiler(&compiler);
    release_vm(&vm);
err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
    release(buf);
    fclose(fp);
}