#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "scanner.h"

int main() {
    FILE *fp = fopen("./test/scanning/numbers.txt", "rb");
    char* buffer = NULL;
    long length;
    if (!fp) {
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = malloc(length);
    fread(buffer, 1, length, fp);

    init_scanner(buffer);
    struct Token token;
    while ((token = next_token()).type != TOKEN_EOF) {
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
        printf(" ");
        for (int i = 0; i < token.length; i++) {
            printf("%c", token.start[i]);
        }
        printf("\n");
    }

    fclose(fp);
}