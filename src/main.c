#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "common.h"
#include "parse.h"
#include "resolve.h"
#include "ty.h"
#include "debug.h"

int main () {
    FILE *fp = fopen("test/in.txt", "rb");
    if (!fp)
        exit(1);
    
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // TODO error if source code contains null bytes
    char *buf = allocate(length+2);
    if (!buf)
        exit(1);
    buf[0] = '\0';
    buf[length+1] = '\0';
    char *source = buf+1;
    fread(source, 1, length, fp);
    struct Arena arena;
    init_arena(&arena);
    struct BlockNode *block = parse(&arena, source);
    if (!block)
        exit(1);

    struct SymTable st;
    init_symtable(&st);
    if (!resolve_names(&st, (struct Node*)block))
        exit(1);

    resolve_tys(&st, (struct Node*)block);
    print_symtable(&st);
    
    free_symtable(&st);
    free_arena(&arena);
    release(buf);
    fclose(fp);
}