#pragma once
#include "scan.h"
#define FLAG_NONE     (0)
#define FLAG_CAPTURED (1 << 1) // variable that a function closes over

struct Symbol {
    struct Span span; // identifier
    u32 flags;
    i32 depth;
    // offset from base of enclosing function -1 initially to indiciate it's unset
    i32 idx;
};

struct SymArr {
    i32 cap;
    i32 cnt;
    struct Symbol *symbols;
};

void init_symbol_arr(struct SymArr *arr);

void release_symbol_arr(struct SymArr *arr);

i32 push_symbol_arr(struct SymArr *arr, struct Symbol sym);