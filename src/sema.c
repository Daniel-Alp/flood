#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "sema.h"

// TODO reset arena stack_pos when traversing tree because current implementation is wasteful of memory
void init_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 8;
    st->symbols = allocate(st->cap * sizeof(struct Symbol));
}

void release_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 0;
    release(st->symbols);
    st->symbols = NULL;
}

// creates a new entry in the symbol table and return the index
u32 push_symtable(struct SymTable *st, struct Span span) {
    if (st->count == st->cap) {
        st->cap *= 2;
        st->symbols = reallocate(st->symbols, st->cap * sizeof(struct Symbol));
    }
    st->symbols[st->count].span = span;
    st->symbols[st->count].flags = SYM_FLAG_NONE;
    st->symbols[st->count].idx = -1;
    st->count++;
    return st->count-1;
}

void init_sema_state(struct SemaState *sema) {
    sema->local_count = 0;
    sema->global_count = 0;
    sema->depth = 0;
    init_errlist(&sema->errlist);
    init_symtable(&sema->st);
}

void release_sema_state(struct SemaState *sema) {
    sema->local_count = 0;
    release_errlist(&sema->errlist);
    release_symtable(&sema->st);
}

static struct Ident *resolve_ident(struct SemaState *sema, struct Span name) {
    for (i32 i = sema->local_count-1; i >= 0; i--) {
        struct Ident *ident = &sema->locals[i];
        if (ident->span.length == name.length && memcmp(ident->span.start, name.start, name.length) == 0) 
            return ident;
    }
    for (i32 i = 0; i <= sema->global_count; i++) {
        struct Ident *ident = &sema->globals[i];
        if (ident->span.length == name.length && memcmp(ident->span.start, name.start, name.length) == 0) 
            return ident;
    }
    return NULL;
} 

static void analyze_node(struct SemaState *sema, struct Node *node);

static void analyze_literal(struct SemaState *sema, struct LiteralNode *node) {
    
}

static void analyze_ident(struct SemaState *sema, struct IdentNode *node) {
    struct Ident *ident = resolve_ident(sema, node->base.span);
    if (!ident) {
        push_errlist(&sema->errlist, node->base.span, "not found in this scope");
        return;
    }
    node->id = ident->id;
}

static void analyze_unary(struct SemaState *sema, struct UnaryNode *node) {
    analyze_node(sema, node->rhs);
}

static void analyze_binary(struct SemaState *sema, struct BinaryNode *node) {
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
        if (sema->st.symbols[id].flags & SYM_FLAG_IMMUTABLE) {
            push_errlist(&sema->errlist, ((struct IdentNode*)node->lhs)->base.span, "cannot reassign variable");
            return;  
        }
    }
    analyze_node(sema, node->lhs);
    analyze_node(sema, node->rhs);
}

static void analyze_fn_call(struct SemaState *sema, struct FnCallNode *node) {
    analyze_node(sema, node->lhs);
    for (i32 i = 0; i < node->arity; i++)
        analyze_node(sema, node->args[i]);
}

static void analyze_block(struct SemaState *sema, struct BlockNode *node) {
    u32 count = sema->local_count;
    sema->depth++;
    for (i32 i = 0; i < node->count; i++)
        analyze_node(sema, node->stmts[i]);
    sema->depth--;
    sema->local_count = count;
}

static void analyze_if(struct SemaState *sema, struct IfNode *node) {
    analyze_node(sema, node->cond);
    analyze_block(sema, node->thn);
    if (node->els)
        analyze_block(sema, node->els);
}

static void analyze_expr_stmt(struct SemaState *sema, struct ExprStmtNode *node) {
    analyze_node(sema, node->expr);
}

static void analyze_print(struct SemaState *sema, struct PrintNode *node) {
    analyze_node(sema, node->expr);
}

static void analyze_return(struct SemaState *sema, struct ReturnNode *node) {
    // TODO if I allow top level statements (apart from function and variable declarations) 
    // do not allow a return statement at the top level
    if (node->expr)
        analyze_node(sema, node->expr);
}

static void analyze_var_decl(struct SemaState *sema, struct VarDeclNode *node) {
    struct Ident *ident = resolve_ident(sema, node->base.span);
    if (ident && ident->depth == sema->depth) {
        push_errlist(&sema->errlist, node->base.span, "redeclared variable");
        return;
    }
    u32 id = push_symtable(&sema->st, node->base.span);
    node->id = id;
    if (node->init)
        analyze_node(sema, node->init);

    if (sema->depth == 0)
        sema->st.symbols[id].flags |= SYM_FLAG_GLOBAL;
    else
        sema->st.symbols[id].flags |= SYM_FLAG_LOCAL;
    // TODO error if more than 256 locals
    sema->locals[sema->local_count].id = id;
    sema->locals[sema->local_count].span = node->base.span;
    sema->locals[sema->local_count].depth = sema->depth;
    sema->local_count++;
}

// function declarations are top-level
static void analyze_fn_decl(struct SemaState *sema, struct FnDeclNode *node) {
    // TODO error if more than 256 globals
    u32 id = push_symtable(&sema->st, node->base.span);

    sema->st.symbols[id].flags |= SYM_FLAG_GLOBAL | SYM_FLAG_IMMUTABLE;

    sema->globals[sema->global_count].id = id;
    sema->globals[sema->global_count].span = node->base.span;
    sema->globals[sema->global_count].depth = 0;
    sema->global_count++;
}

static void analyze_node(struct SemaState *sema, struct Node *node) {
    switch (node->tag) {
    case NODE_LITERAL:   return analyze_literal(sema, (struct LiteralNode*)node);
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

void analyze(struct SemaState *sema, struct ModuleNode *mod) {
    for (i32 i = 0; i < mod->count; i++) 
        analyze_fn_decl(sema, (struct FnDeclNode*)mod->stmts[i]);

    for (i32 i = 0; i < mod->count; i++) {
        // TODO global variable declarations
        struct FnDeclNode *fn = (struct FnDeclNode*)mod->stmts[i];

        // sema->local_count = 0 because function declarations are all top-level
        for (i32 i = 0; i < fn->arity; i++) {
            struct Span span = fn->param_spans[i];
            for (i32 j = 0; j < i; j++) {
                struct Span other_span = fn->param_spans[j];
                if (other_span.length == span.length && memcmp(other_span.start, span.start, span.length) == 0) {
                    push_errlist(&sema->errlist, span, "used as parameter more than once");
                    break;
                }
            }
            u32 id = push_symtable(&sema->st, fn->base.span);
            // TODO error if more than 256 locals
            sema->locals[i].id = id;
            sema->locals[i].depth = 1;
            sema->locals[i].span = span;
        }
        sema->local_count = fn->arity;
        analyze_block(sema, fn->body);
    }
}