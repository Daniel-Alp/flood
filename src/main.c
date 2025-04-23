#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "common.h"
#include "parser.h"
#include "symtable.h"
#include "debug.h"

int main () {
    FILE *fp = fopen("test/in.txt", "rb");
    if (!fp)
        exit(1);
    
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // TODO error if source code contains null bytes
    char *source = allocate(length+2);
    if (!source)
        exit(1);
    source[0] = '\0';
    source++;
    fread(source, 1, length, fp);
    source[length] = '\0';

    struct Arena arena;
    init_arena(&arena);
    struct BlockNode *block = parse(&arena, source);
    if (!block)
        exit(1);

    struct SymTable st;
    init_symtable(&st);
    if (!resolve(&st, (struct Node*)block))
        exit(1);
    print_node((struct Node*)block, 0);
    
    free_symtable(&st);
    free_arena(&arena);
    fclose(fp);
}