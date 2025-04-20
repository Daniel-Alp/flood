#pragma once
#include "parser.h"

struct Symbol {
    u64 key; // address of the symbol name (key = 0 is available slot)
};

// an entry is created for each block in the source
// it is used to lookup symbols declared in a block
// the entries form a parent-pointer tree
struct SymTableEntry {
    u64 key;                      // AST node address the entry was created from (key = 0 is available slot)
    u32 count;
    u32 cap;
    struct Symbol *symbols;       // hash table mapping symbol names to descriptions
    struct SymTableEntry *parent;
};

struct SymTable {
    u32 count;
    u32 cap;
    struct SymTableEntry *blocks; // hash table mapping AST node addresses to symbol table entries
    struct SymTableEntry *cur;
};

void init_symtable(struct SymTable *st);

void free_symtable(struct SymTable *st);

struct SymTableEntry *find_ste(struct SymTableEntry *blocks, u32 cap, u64 key);

void st_insert_ste(struct SymTable *st, u64 key);