#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "parse.h"
#include "debug.h"

int main(int argc, const char **argv) 
{
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("File `%s` does not exist\n", argv[1]);
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

    printf("%s\n", argv[1]);
    printf("%s\n", source);
    struct Parser parser;
    init_parser(&parser);
    struct FileNode *ast = parse(&parser, source);
    if (parser.errlist.cnt > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }
    for (i32 i = 0; i < ast->cnt; i++)
        print_node(ast->stmts[i], 0);
    printf("\n");

err_release_parser:
    release_parser(&parser);
    release(buf);
err_close_fp:
    fclose(fp);
}