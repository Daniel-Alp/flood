#include <stdlib.h>
#include "symtable.h"
#include "../util/memory.h"

#define TABLE_MAX_LOAD 0.5

void init_symtable_entry(struct SymTableEntry *ste) {
    ste->key = 0;
    ste->count = 0;
    ste->cap = 8;
    ste->symbols = allocate(ste->cap * sizeof(struct Symbol));
    for (u32 i = 0; i < ste->cap; i++)
        ste->symbols[i].key = 0;    
}

void free_symtable_entry(struct SymTableEntry *ste) {
    ste->key = 0;
    ste->count = 0;
    ste->cap = 0;
    free(ste->symbols);
    ste->symbols = NULL;
}

void init_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 8;
    st->blocks = allocate(st->cap * sizeof(struct SymTableEntry));
    for (u32 i = 0; i < st->cap; i++)
        init_symtable_entry(&(st->blocks[i]));
    st->cur = NULL;
}

void free_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 0;
    for (u32 i = 0; i < st->cap; i++)
        free_symtable_entry(&(st->blocks[i]));
    free(st->blocks);
    st->blocks = NULL;
    st->cur = NULL;
}

static u64 hash(u64 x) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

// finds first available entry or entry with matching key in hashtable
// precondition: st->count < st->cap 
struct SymTableEntry *find_ste(struct SymTableEntry *blocks, u32 cap, u64 key) {
    u64 i = hash(key) & (cap-1);
    while (true) {
        struct SymTableEntry *ste = &blocks[i];
        if (ste->key == 0 || ste->key == key) 
            return ste;
        i = (i+1) & (cap-1);
    }
}

void st_insert_ste(struct SymTable *st, u64 key) {
    if (st->count + 1 >= st->cap * TABLE_MAX_LOAD) {
        struct SymTableEntry *blocks = allocate(st->cap * 2 * sizeof(struct SymTableEntry));
        
        for (u32 i = 0; i < st->cap * 2; i++)
            init_symtable_entry(&blocks[i]);
        
        for (u32 i = 0; i < st->cap; i++) {
            u64 key = st->blocks[i].key;
            if (key == 0)
                continue;
            struct SymTableEntry *ste = find_ste(blocks, st->cap * 2, key);
            ste->key = key;
        }
        free(st->blocks);
        st->blocks = blocks;
        st->cap *= 2;
    }
    struct SymTableEntry *ste = find_ste(st->blocks, st->cap, key);
    ste->key = key;
    st->count++;
}