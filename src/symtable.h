#pragma once
#include "parser.h"

#define U8_COUNT 256

struct Symbol {
    struct Token name;      
};

struct SymTable {
    i32 count;
    u32 cap;
    struct Symbol *symbols;
};

struct Local {
    struct Token name;
    u32 id;
    i32 depth;
};

struct Resolver {
    struct Local locals[U8_COUNT];
    i32 count;
    i32 depth;
    bool had_error;
};

void init_symtable(struct SymTable *st);

void free_symtable(struct SymTable *st);

bool resolve(struct SymTable *st, struct Node *node);