#pragma once
#include "parse.h"
#define SYM_FLAG_NONE 0
#define SYM_FLAG_INITIALIZED 1
#define MAX_LOCALS 256

struct Symbol {
    struct Span span;
    // NULL means unknown type
    struct TyNode *ty;
    u32 flags;
    // if local, index into stack
    // if global, index into global array
    i32 idx;
};

struct SymTable {
    u32 count;
    u32 cap;
    struct Symbol *symbols;
    // symbol type info lives on this arena
    struct Arena arena;
};

struct Local {
    struct Span span;
    u32 id;
};

struct SemaState {
    u32 count;
    struct Local locals[MAX_LOCALS];
    
    struct ErrList errlist;
    // type info while traversing AST lives on this arena
    struct Arena scratch;
    struct SymTable st;
};

void init_sema_state(struct SemaState *sema);

void release_sema_state(struct SemaState *sema);

void analyze(struct SemaState *sema, struct Node *node);