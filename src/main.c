#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "common.h"
#include "parser.h"
#include "debug.h"

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

    struct Arena arena;
    init_arena(&arena);
    struct BlockExpr *expr = parse(&arena, source);
    if (!expr)
        exit(1);
    print_node((struct Node*)expr, 0);

    free_arena(&arena);
    fclose(fp);
}