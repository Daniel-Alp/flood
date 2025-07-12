#pragma once
#include "symbol.h"
#include "parse.h"

struct SemaState {
    u32 depth;                // level of nestedness while traversing AST
    u32 local_cnt;            // num locals on stack while traversing AST 
    u32 locals[MAX_LOCALS];   // ids of locals on the stack while traversing AST
    struct SymArr *sym_arr;
    struct FnDeclNode *fn;    // fn we are inside of while traversing AST
    struct ErrList errlist;
};

void init_sema_state(struct SemaState *sema, struct SymArr *sym_arr);

void release_sema_state(struct SemaState *sema);

void analyze(struct SemaState *sema, struct FnDeclNode *file);