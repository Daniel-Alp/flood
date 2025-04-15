#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "common.h"
#include "scanner.h"

int main () {
    FILE *fp = fopen("test/in.txt", "rb");
    if (!fp)
        exit(1);
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *source = allocate(length+1);
    if (!source)
        exit(1);
    fread(source, 1, length, fp);
    source[length] = '\0';
    
    struct Scanner scanner;
    init_scanner(&scanner, source);

    struct Token token;
    while((token = next_token(&scanner)).type != TOKEN_EOF)
        printf("%.*s\n", token.length, token.start);

    fclose(fp);
}