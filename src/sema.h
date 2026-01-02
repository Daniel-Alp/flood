#pragma once
#include "symbol.h"
#include "parse.h"
#define MAX_GLOBALS (256)

struct SemaState {
    i32 depth;                // level of nestedness while traversing AST
    i32 local_cnt;            // num locals on stack while traversing AST 
    i32 locals[MAX_LOCALS];   // ids of locals on the stack while traversing AST
    i32 global_cnt;
    i32 globals[MAX_GLOBALS];
    struct SymArr *sym_arr;
    struct FnDeclNode *fn_node;    // fn we are inside of while traversing AST
    struct ErrArr errarr;
};

void init_sema_state(struct SemaState *sema, struct SymArr *sym_arr);

void release_sema_state(struct SemaState *sema);

void analyze(struct SemaState *sema, struct FnDeclNode *file);
