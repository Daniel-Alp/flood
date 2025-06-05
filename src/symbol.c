#include <string.h>
#include "symbol.h"
#include "memory.h"

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