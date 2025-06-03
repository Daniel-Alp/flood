#include <string.h>
#include <stdlib.h>
#include "sema.h"

void init_sema_state(struct SemaState *sema, struct SymArr *sym_arr) 
{
    sema->local_cnt = 0;
    sema->global_cnt = 0;
    sema->depth = 0;
    // compiler does not own sym_arr
    sema->sym_arr = sym_arr;
    init_errlist(&sema->errlist);
}

void release_sema_state(struct SemaState *sema)
{
    sema->local_cnt = 0;
    sema->global_cnt = 0;
    sema->depth = 0;
    sema->sym_arr = NULL;
    release_errlist(&sema->errlist);
}

static struct Ident *resolve_ident(struct SemaState *sema, struct Span span)
{
    for (i32 i = sema->local_cnt-1; i >= 0; i--) {
        struct Ident *ident = &sema->locals[i];
        if (ident->span.length == span.length && memcmp(ident->span.start, span.start, span.length) == 0) 
            return ident;
    }
    for (i32 i = sema->global_cnt-1; i >= 0; i--) {
        struct Ident *ident = &sema->globals[i];
        if (ident->span.length == span.length && memcmp(ident->span.start, span.start, span.length) == 0) 
            return ident;
    }
    return NULL;
}

static void analyze_node(struct SemaState *sema, struct Node *node);

static void analyze_ident(struct SemaState *sema, struct IdentNode *node)
{
    struct Ident *ident = resolve_ident(sema, node->base.span);
    if (!ident) {
        push_errlist(&sema->errlist, node->base.span, "not found in this scope");
        return;
    }
    node->id = ident->id;
}

static void analyze_list(struct SemaState *sema, struct ListNode *node)
{
    for (i32 i = 0; i < node->cnt; i++)
        analyze_node(sema, node->items[i]);
}

static void analyze_unary(struct SemaState *sema, struct UnaryNode *node)
{
    analyze_node(sema, node->rhs);
}

static void analyze_binary(struct SemaState *sema, struct BinaryNode *node) 
{
    if (node->op_tag == TOKEN_EQ && node->lhs->tag != NODE_IDENT) {
        push_errlist(&sema->errlist, node->base.span, "cannot assign to left-hand expression");
    }
    if (node->op_tag == TOKEN_EQ && node->lhs->tag == NODE_IDENT) {
        struct Ident *ident = resolve_ident(sema, ((struct IdentNode*)node->lhs)->base.span);
        if (!ident) {
            push_errlist(&sema->errlist, ((struct IdentNode*)node->lhs)->base.span, "not found in this scope");
            return;
        }
        u32 id = ident->id;
        ((struct IdentNode*)node->lhs)->id = id;
    }
    analyze_node(sema, node->lhs);
    analyze_node(sema, node->rhs);
}

static void analyze_get_prop(struct SemaState *sema, struct GetPropNode *node)
{
    analyze_node(sema, node->lhs);
}

static void analyze_fn_call(struct SemaState *sema, struct FnCallNode *node) 
{
    analyze_node(sema, node->lhs);
    for (i32 i = 0; i < node->arity; i++)
        analyze_node(sema, node->args[i]);
}

static void analyze_block(struct SemaState *sema, struct BlockNode *node) 
{
    u32 count = sema->local_cnt;
    sema->depth++;
    for (i32 i = 0; i < node->cnt; i++)
        analyze_node(sema, node->stmts[i]);
    sema->depth--;
    sema->local_cnt = count;
}

static void analyze_if(struct SemaState *sema, struct IfNode *node) 
{
    analyze_node(sema, node->cond);
    analyze_block(sema, node->thn);
    if (node->els)
        analyze_block(sema, node->els);
}

static void analyze_expr_stmt(struct SemaState *sema, struct ExprStmtNode *node) 
{
    analyze_node(sema, node->expr);
}

static void analyze_print(struct SemaState *sema, struct PrintNode *node) 
{
    analyze_node(sema, node->expr);
}

static void analyze_return(struct SemaState *sema, struct ReturnNode *node) 
{
    if (node->expr)
        analyze_node(sema, node->expr);
}

static void analyze_var_decl(struct SemaState *sema, struct VarDeclNode *node) 
{
    struct Span span = node->base.span;
    struct Ident *ident = resolve_ident(sema, span);
    if (ident && ident->depth == sema->depth) {
        push_errlist(&sema->errlist, span, "redeclared variable");
        return;
    }
    struct Symbol sym = {
        .span = span,
        .flags = sema->depth == 0 ? FLAG_GLOBAL : FLAG_LOCAL
    };
    u32 id = push_symbol_arr(sema->sym_arr, sym);
    node->id = id;
    if (node->init)
        analyze_node(sema, node->init);
    // TODO error if more than 256 locals
    sema->locals[sema->local_cnt].id = id;
    sema->locals[sema->local_cnt].span = span;
    sema->locals[sema->local_cnt].depth = sema->depth;
    sema->local_cnt++;
}

// function declarations are top-level
static void analyze_fn_decl(struct SemaState *sema, struct FnDeclNode *node) 
{
    struct Span span = node->base.span;
    struct Ident *ident = resolve_ident(sema, span);
    if (ident && ident->depth == sema->depth) {
        push_errlist(&sema->errlist, span, "redeclared variable");
        return;
    }
    // TODO error if more than 256 globals
    struct Symbol sym = {
        .span = span,
        .flags = FLAG_GLOBAL,
        .idx = sema->global_cnt
    };
    u32 id = push_symbol_arr(sema->sym_arr, sym);
    node->id = id;
    
    sema->globals[sema->global_cnt].id = id;
    sema->globals[sema->global_cnt].span = span;
    sema->globals[sema->global_cnt].depth = 0;
    sema->global_cnt++;
}

static void analyze_node(struct SemaState *sema, struct Node *node)
{
    switch (node->tag) {
    case NODE_ATOM:      return;
    case NODE_LIST:      analyze_list(sema, (struct ListNode*)node); break;
    case NODE_IDENT:     analyze_ident(sema, (struct IdentNode*)node); break;
    case NODE_UNARY:     analyze_unary(sema, (struct UnaryNode*)node); break;
    case NODE_BINARY:    analyze_binary(sema, (struct BinaryNode*)node); break;
    case NODE_GET_PROP:  analyze_get_prop(sema, (struct GetPropNode*)node); break;
    case NODE_FN_CALL:   analyze_fn_call(sema, (struct FnCallNode*)node); break;
    case NODE_BLOCK:     analyze_block(sema, (struct BlockNode*)node); break;
    case NODE_IF:        analyze_if(sema, (struct IfNode*)node); break;
    case NODE_EXPR_STMT: analyze_expr_stmt(sema, (struct ExprStmtNode*)node); break;
    case NODE_RETURN:    analyze_return(sema, (struct ReturnNode*)node); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     analyze_print(sema, (struct PrintNode*)node); break;
    case NODE_VAR_DECL:  analyze_var_decl(sema, (struct VarDeclNode*)node); break;
    }
}

void analyze(struct SemaState *sema, struct FileNode *file)
{
    for (i32 i = 0; i < file->cnt; i++) 
        analyze_fn_decl(sema, (struct FnDeclNode*)file->stmts[i]);

    for (i32 i = 0; i < file->cnt; i++) {
        // TODO global variable declarations
        struct FnDeclNode *fn = (struct FnDeclNode*)file->stmts[i];

        // sema->local_count = 0 because function declarations are all top-level
        for (i32 i = 0; i < fn->arity; i++) {
            struct Span span = fn->params[i].base.span;
            for (i32 j = 0; j < i; j++) {
                struct Span other_span = fn->params[j].base.span;
                if (other_span.length == span.length && memcmp(other_span.start, span.start, span.length) == 0) {
                    push_errlist(&sema->errlist, span, "used as parameter more than once");
                    break;
                }
            }
            struct Symbol param = {
                .span = span,
                .flags = FLAG_LOCAL
            };
            u32 id = push_symbol_arr(sema->sym_arr, param);
            fn->params[i].id = id;
            // TODO error if more than 256 locals
            sema->locals[i].id = id;
            sema->locals[i].depth = 1;
            sema->locals[i].span = span;
        }
        sema->local_cnt = fn->arity;
        analyze_block(sema, fn->body);
    }
}