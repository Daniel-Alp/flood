#pragma once
#include "scan.h"
#define FLAG_NONE   (0)
#define FLAG_LOCAL  (1)
#define FLAG_GLOBAL (1 << 1)

struct Symbol {
    struct Span span; // identifier
    u32 hash;         // hash of identifier
    // index into value stack if local (determined during compilation)
    // index into globals array if global (determined during semantic phase)
    u32 idx;         
    u32 flags;
};

struct SymArr {
    u32 cap;
    u32 cnt;
    struct Symbol *symbols;
};

void init_symbol_arr(struct SymArr *arr);

void release_symbol_arr(struct SymArr *arr);

u32 push_symbol_arr(struct SymArr *arr, struct Symbol sym);