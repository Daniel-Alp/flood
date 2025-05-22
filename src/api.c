#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "parse.h"
#include "sema.h"
#include "compile.h"

// parse and compile the file and its imports, update the files array of the VM
// invoke main method of the file (if it is defined)
enum InterpResult do_file(struct VM *vm, const char *path)
{
    // TODO handle imports
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        printf("File `%s` does not exist\n", path);
        goto err_close_fp;
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

    struct Parser parser;
    init_parser(&parser);
    struct FileNode *ast = parse(&parser, source);
    if (parser.errlist.cnt > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }

    struct SymArr sym_arr;
    init_symbol_arr(&sym_arr);

    struct SemaState sema;
    init_sema_state(&sema, &sym_arr);
    analyze(&sema, ast);
    if (sema.errlist.cnt > 0) {
        print_errlist(&sema.errlist);
        release_symbol_arr(&sym_arr);
        goto err_release_sema_state;
    } 

    struct Compiler compiler;
    // TODO global count should start from VM global count
    init_compiler(&compiler, &sym_arr);

    struct FnObj *fn = compile_file(vm, &compiler, ast);
    // invoke main function if defined
    if (compiler.main_fn_idx != -1) {
        emit_byte(&fn->chunk, OP_GET_GLOBAL, 1);
        emit_byte(&fn->chunk, compiler.main_fn_idx, 1);
        emit_byte(&fn->chunk, OP_CALL, 1);
        emit_byte(&fn->chunk, 0, 1);
    }
    emit_byte(&fn->chunk, OP_NIL, 1);
    emit_byte(&fn->chunk, OP_RETURN, 1);

    // create the sym_map for the file and add it to the files array
    struct SymMap sym_map;
    init_symbol_map(&sym_map);
    for (i32 i = 0; i < ast->cnt; i++) {
        // TODO global variables
        struct FnDeclNode *fn_decl = (struct FnDeclNode*)ast->stmts[i];
        struct Symbol sym = sym_arr.symbols[fn_decl->id];
        struct Span span = sym.span;
        // copy the fn decl span, because we need the global symbol spans but we release buf before running the VM 
        struct Span span_cpy = {
            .start = strncpy(push_arena(&vm->arena, span.length * sizeof(char)), span.start, span.length),
            .length = span.length
        };
        put_symbol_map(&sym_map, span_cpy, sym.idx, sym.flags);
    }
    const char *normalized_path = realpath(path, NULL);
    push_file_array(&vm->files, normalized_path, sym_map);

    release_symbol_arr(&sym_arr);
    release_sema_state(&sema);
    release_parser(&parser);
    release(buf);
    fclose(fp);

    // TODO have global count start from the vm global count
    for (i32 i = 0; i < compiler.global_cnt; i++)
        push_val_array(&vm->globals, NIL_VAL);

    // disassemble_chunk(&fn->chunk, fn->name);
    // for (i32 i = 0; i < fn->chunk.constants.cnt; i++) {
    //     Value val = fn->chunk.constants.values[i];
    //     if (IS_FN(val))
    //         disassemble_chunk(&AS_FN(val)->chunk, AS_FN(val)->name);
    // }
    return run_vm(vm, fn);

err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
    release(buf);
err_close_fp:
    fclose(fp);
    return INTERP_COMPILE_ERR;
}

// TEMP find the global index of a variable
i32 get_global(struct VM *vm, const char *file_path, const char *name)
{
    char *path = allocate(strlen(vm->source_dir) + strlen(file_path) + 1);
    strcpy(path, vm->source_dir);
    strcat(path, file_path);
    
    char *normalized_path = realpath(path, NULL);
    for (i32 i = 0; i < vm->files.cnt; i++) {
        if (strcmp(normalized_path, vm->files.paths[i]) == 0) {
            struct SymMap sym_map = vm->files.sym_maps[i];
            struct Span span = {
                .start = name,
                .length = strlen(name)
            };
            struct Symbol *sym = get_symbol_map_slot(sym_map.symbols, sym_map.cap, span, hash_string(span));
            free(normalized_path);
            release(path);
            if (sym->span.length == 0)
                return -1;
            else 
                return sym->idx;
        }
    }
    free(normalized_path);
    release(path);
    return -1;
}