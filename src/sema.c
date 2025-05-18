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
    struct Ident *ident = resolve_ident(sema, node->base.span);
    if (ident && ident->depth == sema->depth) {
        push_errlist(&sema->errlist, node->base.span, "redeclared variable");
        return;
    }
    struct Symbol sym;
    sym.span = node->base.span;
    if (sema->depth == 0)
        sym.flags = FLAG_GLOBAL;
    else
        sym.flags = FLAG_LOCAL;

    u32 id = push_symbol_arr(sema->sym_arr, sym);
    node->id = id;
    if (node->init)
        analyze_node(sema, node->init);
    // TODO error if more than 256 locals
    sema->locals[sema->local_cnt].id = id;
    sema->locals[sema->local_cnt].span = sym.span;
    sema->locals[sema->local_cnt].depth = sema->depth;
    sema->local_cnt++;
}

// function declarations are top-level
static void analyze_fn_decl(struct SemaState *sema, struct FnDeclNode *node) 
{
    // TODO error if more than 256 globals
    struct Symbol sym = {
        .span = node->base.span,
        .flags = FLAG_GLOBAL
    };
    u32 id = push_symbol_arr(sema->sym_arr, sym);
    sema->globals[sema->global_cnt].id = id;
    sema->globals[sema->global_cnt].span = sym.span;
    sema->globals[sema->global_cnt].depth = 0;
    sema->global_cnt++;
}

static void analyze_node(struct SemaState *sema, struct Node *node)
{
    switch (node->tag) {
    case NODE_LITERAL:   return;
    case NODE_IDENT:     return analyze_ident(sema, (struct IdentNode*)node);
    case NODE_UNARY:     return analyze_unary(sema, (struct UnaryNode*)node);
    case NODE_BINARY:    return analyze_binary(sema, (struct BinaryNode*)node);
    case NODE_FN_CALL:   return analyze_fn_call(sema, (struct FnCallNode*)node);
    case NODE_BLOCK:     return analyze_block(sema, (struct BlockNode*)node);
    case NODE_IF:        return analyze_if(sema, (struct IfNode*)node);
    case NODE_EXPR_STMT: return analyze_expr_stmt(sema, (struct ExprStmtNode*)node);
    case NODE_RETURN:    return analyze_return(sema, (struct ReturnNode*)node);
    // TEMP remove when we add functions
    case NODE_PRINT:     return analyze_print(sema, (struct PrintNode*)node);
    case NODE_VAR_DECL:  return analyze_var_decl(sema, (struct VarDeclNode*)node);
    }
}

void analyze(struct SemaState *sema, struct ModuleNode *mod)
{
    for (i32 i = 0; i < mod->cnt; i++) 
        analyze_fn_decl(sema, (struct FnDeclNode*)mod->stmts[i]);

    for (i32 i = 0; i < mod->cnt; i++) {
        // TODO global variable declarations
        struct FnDeclNode *fn = (struct FnDeclNode*)mod->stmts[i];

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
                .span = fn->params[i].base.span,
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