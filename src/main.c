#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "common.h"
#include "parse.h"
#include "debug.h"
#include "error.h"

int main () {
    FILE *fp = fopen("test/in.txt", "rb");
    if (!fp)
        exit(1);
    
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // TODO error if source code contains null bytes
    char *buf = allocate(length+2);
    if (!buf) {
        release(buf);
        fclose(fp);
        exit(1);
    }
    buf[0] = '\0';
    buf[length+1] = '\0';
    char *source = buf+1;
    fread(source, 1, length, fp);

    struct Scanner scanner;
    struct Parser parser;
    init_parser(&parser, source);
    parse(&parser);
    if (parser.errlist.count > 0) {
        print_parse_errs(&parser.errlist);
        release_parser(&parser);
        release(buf);
        fclose(fp);
        exit(1);
    }
    print_node(parser.ast, 0);
    release_parser(&parser);
    release(buf);
    fclose(fp);
}