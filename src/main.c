#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "../util/arena.h"
#include "scanner.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "debug.h"

int main () {
    FILE *fp = fopen("/home/alp/Projects/rhubarb/test/test.txt", "rb");
    char *source = NULL;
    u64 length;
    if (!fp) {
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    source = allocate(length);
    if (!source) {
        exit(1);
    }    
    fread(source, 1, length, fp);

    struct Scanner scanner;
    init_scanner(&scanner, source);

    struct Token tk;
    while ((tk = next_token(&scanner)).type != TOKEN_EOF) {
        switch(tk.type) {
            case TOKEN_PLUS:            printf("TOKEN_PLUS"); break;
            case TOKEN_MINUS:           printf("TOKEN_MINUS"); break;
            case TOKEN_STAR:            printf("TOKEN_STAR"); break;
            case TOKEN_SLASH:           printf("TOKEN_SLASH"); break;
            case TOKEN_EQUAL:           printf("TOKEN_EQUAL"); break;
            case TOKEN_LESS_EQUAL:      printf("TOKEN_LESS_EQUAL"); break;
            case TOKEN_GREATER_EQUAL:   printf("TOKEN_GREATER_EQUAL"); break;
            case TOKEN_LESS:            printf("TOKEN_LESS"); break;
            case TOKEN_GREATER:         printf("TOKEN_GREATER"); break;
            case TOKEN_EQUAL_EQUAL:     printf("TOKEN_EQUAL_EQUAL"); break;
            case TOKEN_AND:             printf("TOKEN_AND"); break;
            case TOKEN_OR:              printf("TOKEN_OR"); break;
            case TOKEN_NOT:             printf("TOKEN_NOT"); break;
            case TOKEN_NOT_EQUAL:       printf("TOKEN_NOT_EQUAL"); break;
            case TOKEN_NUMBER:          printf("TOKEN_NUMBER"); break;
            case TOKEN_IDENTIFIER:      printf("TOKEN_IDENTIFIER"); break;
            case TOKEN_TRUE:            printf("TOKEN_TRUE"); break;
            case TOKEN_FALSE:           printf("TOKEN_FALSE"); break;
            case TOKEN_LEFT_PAREN:      printf("TOKEN_LEFT_PAREN"); break;
            case TOKEN_RIGHT_PAREN:     printf("TOKEN_RIGHT_PAREN"); break;
            case TOKEN_LEFT_BRACE:      printf("TOKEN_LEFT_BRACE"); break;
            case TOKEN_RIGHT_BRACE:     printf("TOKEN_RIGHT_BRACE"); break;
            case TOKEN_SEMICOLON:       printf("TOKEN_SEMICOLON"); break;
            case TOKEN_IF:              printf("TOKEN_IF"); break;
            case TOKEN_ELSE:            printf("TOKEN_ELSE"); break;
            case TOKEN_VAR:             printf("TOKEN_VAR"); break;
            case TOKEN_EOF:             printf("TOKEN_EOF"); break;
            case TOKEN_ERROR:           printf("TOKEN_ERROR"); break;
        }
        printf("\n%.*s\n", tk.length, tk.start);
    }

    // struct Arena arena;
    // init_arena(&arena);
    // struct Expr *expr = parse(&arena, source);
    // if (!expr) {
    //     exit(1);    
    // }
    
    // struct Chunk chunk;
    // init_chunk(&chunk);
    // compile(&chunk, expr);
    // disassemble_chunk(&chunk);
    // printf("\n");

    // struct VM vm;
    // init_vm(&vm, chunk);
    // run(&vm);

    fclose(fp);
    // free_chunk(&chunk);
    // free_arena(&arena);
}