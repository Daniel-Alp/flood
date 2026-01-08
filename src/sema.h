#pragma once
#include "dynarr.h"
#include "error.h"
#include "parse.h"

#define MAX_GLOBALS   (256)
#define FLAG_NONE     (0)
#define FLAG_CAPTURED (1 << 1) // variable that a function closes over
#define FLAG_INIT     (1 << 2) // constructor

// TODO add back special case (recursive closure case) and profile

struct Ident {
    const Span span; // identifier
    u32 flags;
    const i32 depth;
    // if local variable, offset from base of the function the variable was declared in
    // if global variable, offset into globals array
    const i32 idx;
};

struct SemaCtx {
    i32 depth; // depth of innermost scope while traversing AST
    i32 local_cnt;
    i32 locals[MAX_LOCALS]; // ids of locals in scope while traversing AST
    i32 global_cnt;
    i32 globals[MAX_GLOBALS]; // ids of globals
    Dynarr<Ident> &idarr;
    FnDeclNode *fn_node; // fn we are in while traversing AST
    Dynarr<ErrMsg> &errarr;

    static void analyze(ModuleNode &node, Dynarr<Ident> &idarr, Dynarr<ErrMsg> &errarr);

private:
    SemaCtx(Dynarr<Ident> &idarr, Dynarr<ErrMsg> &errarr)
        : depth(0), local_cnt(0), global_cnt(0), idarr(idarr), fn_node(nullptr), errarr(errarr)
    {
    }
};
