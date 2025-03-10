#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "scanner.h"

int main () {
    FILE *fp = fopen("./test/scanning/numbers.txt", "rb");
    char* source = NULL;
    u64 length;
    if (!fp) {
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    source = allocate(length);
    fread(source, 1, length, fp);

    struct Scanner scanner;
    init_scanner(&scanner, source);

    struct Token token;
    while ((token = next_token(&scanner)).type != TOKEN_EOF) {
        switch(token.type) {
            case TOKEN_PLUS:
                printf("PLUS");
                break;
            case TOKEN_MINUS:
                printf("MINUS");
                break;
            case TOKEN_STAR:
                printf("STAR");
                break;
            case TOKEN_SLASH:
                printf("SLASH");
                break;
            case TOKEN_LEFT_PAREN:
                printf("LEFT_PAREN");
                break;
            case TOKEN_RIGHT_PAREN:
                printf("RIGHT_PAREN");
                break;
            case TOKEN_NUMBER:
                printf("NUMBER");
                break;
            case TOKEN_ERROR:
                printf("ERROR");
                break;
        }
        printf(" %.*s\n", token.length, token.start);        
    }

    fclose(fp);
}