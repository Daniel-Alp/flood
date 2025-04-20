#include <stdlib.h>
#include <stdio.h>
#include "../util/memory.h"
#include "common.h"
#include "parser.h"
#include "symtable.h"
#include "debug.h"

int main () {
    // FILE *fp = fopen("test/in.txt", "rb");
    // if (!fp)
    //     exit(1);
    // fseek(fp, 0, SEEK_END);
    // u64 length = ftell(fp);
    // fseek(fp, 0, SEEK_SET);
    // char *source = allocate(length+1);
    // if (!source)
    //     exit(1);
    // fread(source, 1, length, fp);
    // source[length] = '\0';

    // struct Arena arena;
    // init_arena(&arena);
    // struct BlockExpr *expr = parse(&arena, source);
    // if (!expr)
    //     exit(1);
    // print_node((struct Node*)expr, 0);

    u64 keys[1000];
    u64 x = 0;
    for (u32 i = 0; i < 1000; i++) {
        uint64_t z = (x += 0x9e3779b97f4a7c15);
	    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	    z = z ^ (z >> 31);
        keys[i] = z;
    }

    struct SymTable st;
    init_symtable(&st);
    for (u32 i = 0; i < 1000; i++) {
        st_insert_ste(&st, keys[i]);
    }

    for (u32 i = 0; i < 1000; i++) {
        if (find_ste(st.blocks, st.cap, keys[i])->key != keys[i])
            printf("Error\n"); 
    }

    free_symtable(&st);

    // u64 key1 = 1;
    // struct SymTableEntry *ste = st_find_ste(&st, key1);
    // printf("key: %ld\n", ste->key);
    // st_insert_ste(&st, key1);
    // u64 key2 = 
    // printf("key: %ld\n", ste->key);

    // free_arena(&arena);
    // fclose(fp);
}