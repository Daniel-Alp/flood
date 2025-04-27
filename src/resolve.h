#pragma once
#include "parse.h"
#include "ty.h"

#define U8_COUNT 256

struct Symbol {
    struct Span span;
    struct Ty ty;     
};

struct SymTable {
    u32 count;
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

bool resolve_names(struct SymTable *st, struct Node *node);