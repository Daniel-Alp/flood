#include <string.h>
#include "symbol.h"
#include "memory.h"
#define MAP_LOAD_FACTOR (0.75)

void init_symbol_map(struct SymMap *map) 
{
    map->cnt = 0;
    map->cap = 8;
    map->symbols = allocate(map->cap * sizeof(struct Symbol));
    for (i32 i = 0; i < map->cap; i++) {
        map->symbols[i].span.length = 0;
        map->symbols[i].span.start = NULL;
    }
}

void release_symbol_map(struct SymMap *map) 
{
    map->cnt = 0;
    map->cap = 0;
    release(map->symbols);
    map->symbols = NULL;
}

u32 hash_string(struct Span key)
{
    u32 hash = 2166136261u;
    for (i32 i = 0; i < key.length; i++) {
      hash ^= (u8)key.start[i];
      hash *= 16777619;
    }
    return hash;
}

// get slot matching key or empty slot
// precondition: space available
struct Symbol *get_symbol_map_slot(struct Symbol *symbols, u32 cap, struct Span key, u32 hash) 
{
    u32 i = hash & (cap-1);
    while (true) {
        struct Symbol *sym = &symbols[i];
        if (sym->span.length == 0 
            || (hash == sym->hash 
                && key.length == sym->span.length 
                && memcmp(key.start, sym->span.start, key.length)) == 0)
            return sym;
        i = (i+1) & (cap-1);
    }
}

void put_symbol_map(struct SymMap *map, struct Span key, u32 idx, u32 flags) 
{
    if (map->cnt * MAP_LOAD_FACTOR >= map->cap) {
        map->cap *= 2;
        struct Symbol *symbols = allocate(map->cap * sizeof(struct Symbol));
        for (i32 i = 0; i < map->cap; i++) {
            map->symbols[i].span.length = 0;
            map->symbols[i].span.start = NULL;
        }
        for (i32 i = 0; i < map->cnt; i++) {
            u32 hash = hash_string(key);
            struct Symbol *sym = get_symbol_map_slot(symbols, map->cap, map->symbols[i].span, hash);
            memcpy(sym, &map->symbols[i], sizeof(struct Symbol));
        }
        release(map->symbols);
        map->symbols = symbols;
    }
    u32 hash = hash_string(key);
    struct Symbol *sym = get_symbol_map_slot(map->symbols, map->cap, key, hash);
    sym->span = key;
    sym->hash = hash;
    sym->idx = idx;
    sym->flags = flags;
    map->cnt++;
}

void init_symbol_arr(struct SymArr *arr) 
{
    arr->cnt = 0;
    arr->cap = 8;
    arr->symbols = allocate(arr->cap * sizeof(struct Symbol));
}

void release_symbol_arr(struct SymArr *arr)
{
    arr->cnt = 0;
    arr->cap = 0;
    release(arr->symbols);
    arr->symbols = NULL;
}

u32 push_symbol_arr(struct SymArr *arr, struct Symbol sym) 
{
    if (arr->cnt == arr->cap) {
        arr->cap *= 2;
        arr->symbols = reallocate(arr->symbols, arr->cap * sizeof(struct Symbol));
    }
    arr->symbols[arr->cnt] = sym;
    arr->cnt++;
    return arr->cnt-1;
}