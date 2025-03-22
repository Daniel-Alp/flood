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
    for (int i = 0; i <= 15; i++) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "/home/alp/Projects/rhubarb/test/parsing/test%d.txt", i);
        
        FILE *fp = fopen(buffer, "rb");
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

        struct Arena arena;
        init_arena(&arena);
        printf("TEST %d\n", i);
        struct BlockExpr *expr = parse(&arena, source);
        if (!expr) {
            printf("\n========================================\n");
            continue;
        }
        print_node((struct Node*)expr, 0);
        printf("\n========================================\n");

        free_arena(&arena);
        fclose(fp);
    }


    


    // struct Chunk chunk;
    // init_chunk(&chunk);
    // compile(&chunk, expr);
    // disassemble_chunk(&chunk);
    // printf("\n");

    // struct VM vm;
    // init_vm(&vm, chunk);
    // run(&vm);

    // free_chunk(&chunk);
}