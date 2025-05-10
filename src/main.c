#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "common.h"
#include "parse.h"
#include "sema.h"
#include "compile.h"
#include "vm.h"
#include "debug.h"
#include "error.h"

int main (int argc, char **argv) {
    if (argc != 2)
        exit(1);

    // printf("%s\n", argv[1]);
    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
        exit(1);
    
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // TODO error if source code contains null bytes
    char *buf = allocate(length+2);
    if (!buf)
        goto err_release_buf;
    buf[0] = '\0';
    buf[length+1] = '\0';
    char *source = buf+1;
    fread(source, 1, length, fp);
    printf("%s\n", argv[1]);
    printf("%s\n\n", source);

    struct Parser parser;
    init_parser(&parser, source);
    parse(&parser);

    if (parser.errlist.count > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }

    struct FileNode *module = (struct FileNode*)parser.ast;
    for (i32 i = 0; i < module->count; i++) {
        print_node(module->stmts[i], 0);
        printf("\n");
    }

    // for now just compile and execute the body of the first function
    // this is a hack but needed for testing
    struct Node *body = (struct Node*)((struct FnDeclNode*)module->stmts[0])->body;

    struct SemaState sema;
    init_sema_state(&sema);
    analyze(&sema, body);

    if (sema.errlist.count > 0) {
        print_errlist(&sema.errlist);
        goto err_release_sema_state;
    }

    struct Compiler compiler;
    init_compiler(&compiler);
    compile(&compiler, body, &sema.st);

    disassemble_chunk(&compiler.chunk);
    run(&compiler.chunk);

    release_compiler(&compiler);
err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
err_release_buf:
    release(buf);
    fclose(fp);
}