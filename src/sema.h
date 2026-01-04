#pragma once
#include "dynarr.h"
#include "error.h"
#include "parse.h"

#define MAX_GLOBALS (256)
#define FLAG_NONE     (0)
#define FLAG_CAPTURED (1 << 1) // variable that a function closes over
#define FLAG_INIT     (1 << 3) // identifier is a constructor

struct Symbol {
    Span span; // identifier
    u32 flags;
    i32 depth;
    // offset from base of enclosing function -1 initially to indiciate it's unset
    i32 idx;
};

struct SemaCtx {
    i32 depth;                // level of nestedness while traversing AST
    i32 local_cnt;            // num locals on stack while traversing AST 
    i32 locals[MAX_LOCALS];   // ids of locals on the stack while traversing AST
    i32 global_cnt;
    i32 globals[MAX_GLOBALS];
    // struct SymArr *sym_arr;
    Dynarr<Symbol> &symarr;
    FnDeclNode *fn_node;      // fn we are inside of while traversing AST
    Dynarr<ErrMsg> &errarr;

    static void analyze(Node &node, Dynarr<Symbol> &symarr, Dynarr<ErrMsg> &errarr);
private:
    SemaCtx(Node &node, Dynarr<Symbol> &symarr, Dynarr<ErrMsg> &errarr)
        : depth(0)
        , local_cnt(0)
        , global_cnt(0)
        , symarr(symarr)
        , fn_node(static_cast<FnDeclNode*>(&node))
        , errarr(errarr)
    {}

    ~SemaCtx()
    {
        depth = 0;
        local_cnt = 0;
        global_cnt = 0;
    }
};
// void analyze(struct SemaCtx *s, struct FnDeclNode *file);

// void analyze(struct SemaCtx *s, struct FnDeclNode *file);

// void init_sema_state(struct SemaCtx *sema, struct SymArr *sym_arr);

// void release_sema_state(struct SemaCtx *sema);

// void analyze(struct SemaCtx *s, struct FnDeclNode *file);
