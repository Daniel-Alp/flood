#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "memory.h"
#include "parse.h"
#include "error.h"

int main(int argc, const char **argv) {
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
    if (parser.errlist.count > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }

err_release_parser:
    release_parser(&parser);
    release(buf);
    fclose(fp);
}