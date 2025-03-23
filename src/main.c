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
    FILE *fp = fopen("test/test.txt", "rb");
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
    struct BlockExpr *expr = parse(&arena, source);
    if (!expr) {
        exit(1);
    }
    print_node((struct Node*)expr, 0);
    printf("\n");

    free_arena(&arena);
    fclose(fp);
}