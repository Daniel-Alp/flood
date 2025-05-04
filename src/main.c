#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "common.h"
#include "parse.h"
#include "sema.h"
#include "debug.h"
#include "error.h"

int main (int argc, char **argv) {
    if (argc != 2)
        exit(1);

    printf("%s\n", argv[1]);
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

    struct Scanner scanner;
    struct Parser parser;
    init_parser(&parser, source);
    parse(&parser);

    if (parser.errlist.count > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }

    struct SemaState sema;
    init_sema_state(&sema);
    analyze(&sema, parser.ast);

    if (sema.errlist.count > 0) {
        print_errlist(&sema.errlist);
        goto err_release_sema_state;
    }

    print_node(parser.ast, 0);

err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
err_release_buf:
    release(buf);
    fclose(fp);
}