#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "../util/arena.h"
#include "scanner.h"
#include "parser.h"
#include "debug.h"

int main () {
    FILE *fp = fopen("/home/alp/Projects/rhubarb/test/scanning/numbers.txt", "rb");
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
    arena_init(&arena);
    struct Expr *expr = parse(&arena, source);
    print_expr(expr, 0);
    printf("\n");
    
    free(arena.stack_memory);
    fclose(fp);
}