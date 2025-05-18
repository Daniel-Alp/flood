#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "memory.h"
#include "error.h"
#include "debug.h"
#include "parse.h"
#include "sema.h"
#include "compile.h"
#include "vm.h"

int main(int argc, const char **argv) 
{
    if (argc != 2) {
        printf("Usage: ./flood [script]");
        exit(1);
    }
    const char *path = argv[1];
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        printf("File `%s` does not exist\n", path);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp); 
    fseek(fp, 0, SEEK_SET);
    char *buf = allocate(length+2);
    if (!buf) {
        printf("Failed to allocate buffer for file `%s`\n", path);
        exit(1);
    }
    buf[0] = '\0';
    buf[length+1] = '\0';
    char *source = buf+1;
    // TODO check for null bytes
    fread(source, 1, length, fp);

    printf("%s\n", argv[1]);
    struct Parser parser;
    init_parser(&parser);
    struct ModuleNode *mod = parse(&parser, source);
    if (parser.errlist.cnt > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }

    struct SymArr sym_arr;
    init_symbol_arr(&sym_arr);

    struct SemaState sema;
    init_sema_state(&sema, &sym_arr);
    analyze(&sema, mod);
    
    if (sema.errlist.cnt > 0) {
        print_errlist(&sema.errlist);
        release_symbol_arr(&sym_arr);
        goto err_release_sema_state;
    }

    struct Compiler compiler;
    init_compiler(&compiler, &sym_arr);

    struct FnObj *fn = compile_module(&compiler, mod);

    struct VM vm;
    init_vm(&vm);
    init_val_array(&vm.globals);
    for (i32 i = 0; i < compiler.global_cnt; i++)
        push_val_array(&vm.globals, NIL_VAL);
    // HACK
    // invoke main function after function declarations
    // TODO handle main function not existing
    // TODO error if declaration of main function takes arguments
    emit_byte(&fn->chunk, OP_GET_GLOBAL);
    emit_byte(&fn->chunk, 1); // HACK assume main function is at this index
    emit_byte(&fn->chunk, OP_CALL);
    emit_byte(&fn->chunk, 0);
    emit_byte(&fn->chunk, OP_NIL);
    emit_byte(&fn->chunk, OP_RETURN);

    disassemble_chunk(&fn->chunk, fn->span);
    for (i32 i = 0; i < fn->chunk.constants.cnt; i++) {
        Value val = fn->chunk.constants.values[i];
        if (IS_FN(val))
            disassemble_chunk(&AS_FN(val)->chunk, AS_FN(val)->span);
    }

    run_vm(&vm, fn);    

err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
    release(buf);
    fclose(fp);
}