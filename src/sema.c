#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "sema.h"

// TODO reset arena stack_pos when traversing tree because current implementation is wasteful of memory

void init_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 8;
    st->symbols = allocate(st->cap * sizeof(struct Symbol));
    init_arena(&st->arena);
}

void release_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 0;
    release(st->symbols);
    st->symbols = NULL;
    release_arena(&st->arena);
}

// creates a new entry in the symbol table and return the index
u32 push_symtable(struct SymTable *st, struct Span span) {
    if (st->count == st->cap) {
        st->cap *= 2;
        st->symbols = reallocate(st->symbols, st->cap * sizeof(struct Symbol));
    }
    st->symbols[st->count].ty = NULL;
    st->symbols[st->count].span = span;
    st->symbols[st->count].flags = SYM_FLAG_NONE;
    st->count++;
    return st->count-1;
}

void init_sema_state(struct SemaState *sema) {
    sema->count = 0;
    init_errlist(&sema->errlist);
    init_arena(&sema->scratch);
    init_symtable(&sema->st);
}

void release_sema_state(struct SemaState *sema) {
    sema->count = 0;
    release_errlist(&sema->errlist);
    release_arena(&sema->scratch);
    release_symtable(&sema->st);
}

static struct Local *resolve_ident(struct SemaState *sema, struct Span name) {
    for (i32 i = sema->count-1; i >= 0; i--) {
        struct Local *loc = &sema->locals[i];
        if (loc->span.length == name.length && memcmp(loc->span.start, name.start, name.length) == 0) 
            return loc;
    }
    return NULL;
} 

static struct TyNode *analyze_node(struct SemaState *sema, struct Node *node);

static struct TyNode *analyze_literal(struct SemaState *sema, struct LiteralNode *node) {
    switch(node->lit_tag) {
    case TOKEN_NUMBER:
        return mk_primitive_ty(&sema->scratch, TY_NUM);
    case TOKEN_TRUE:
    case TOKEN_FALSE:
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    }
}

static struct TyNode *analyze_ident(struct SemaState *sema, struct IdentNode *node) {
    struct Local *loc = resolve_ident(sema, node->base.span);
    // TODO only error on first occurence of not in scope
    if (!loc) {
        push_errlist(&sema->errlist, node->base.span, "not found in this scope");
        return mk_primitive_ty(&sema->scratch, TY_ERR);
    }
    struct Symbol symbol = sema->st.symbols[loc->id];
    if (!symbol.ty) {
        push_errlist(&sema->errlist, node->base.span, "unknown type");
        return mk_primitive_ty(&sema->scratch, TY_ERR);
    }
    if ((symbol.flags & SYM_FLAG_INITIALIZED) == 0) {
        push_errlist(&sema->errlist, node->base.span, "used before initialization");
        return mk_primitive_ty(&sema->scratch, TY_ERR);
    }
    return cpy_ty(&sema->scratch, symbol.ty);
}

static struct TyNode *analyze_unary(struct SemaState *sema, struct UnaryNode *node) {
    struct TyNode *ty = analyze_node(sema, node->rhs);
    if (((node->op_tag == TOKEN_MINUS && ty->tag != TY_NUM) || (node->op_tag == TOKEN_NOT && ty->tag != TY_BOOL))
        && ty->tag != TY_ERR) {
        push_errlist(&sema->errlist, node->base.span, "cannot apply operator");
        return mk_primitive_ty(&sema->scratch, TY_ERR);
    }
    return ty;
}

static struct TyNode *analyze_binary(struct SemaState *sema, struct BinaryNode *node) {
    if ((node->op_tag == TOKEN_EQ 
        || node->op_tag == TOKEN_PLUS_EQ 
        || node->op_tag == TOKEN_MINUS_EQ
        || node->op_tag == TOKEN_STAR_EQ
        || node->op_tag == TOKEN_SLASH_EQ)
        && node->lhs->tag != NODE_IDENT) {
        push_errlist(&sema->errlist, node->base.span, "cannot assign to left-hand expression");
        return mk_primitive_ty(&sema->scratch, TY_ERR);
    }

    struct TyNode *ty_rhs = analyze_node(sema, node->rhs);
    struct TyNode *ty_lhs = NULL;
    // type inference. For example,
    //      var x;
    //      x = 5;
    if (node->op_tag == TOKEN_EQ && node->lhs->tag == NODE_IDENT) {
        struct Local *loc = resolve_ident(sema, ((struct IdentNode*)node->lhs)->base.span);
        if (!loc) {
            push_errlist(&sema->errlist, ((struct IdentNode*)node->lhs)->base.span, "not found in this scope");
            return mk_primitive_ty(&sema->scratch, TY_ERR);
        }

        u32 id = loc->id;
        ((struct IdentNode*)node->lhs)->id = id;

        if (!sema->st.symbols[id].ty)
            sema->st.symbols[id].ty = cpy_ty(&sema->st.arena, ty_rhs); 
        
        ty_lhs = cpy_ty(&sema->scratch, sema->st.symbols[id].ty);
        sema->st.symbols[id].flags |= SYM_FLAG_INITIALIZED;
    } else {
        ty_lhs = analyze_node(sema, node->lhs);
    }

    switch(node->op_tag) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PLUS_EQ:
    case TOKEN_MINUS_EQ:
    case TOKEN_STAR_EQ:
    case TOKEN_SLASH_EQ:
        if ((ty_lhs->tag != TY_NUM || ty_rhs->tag != TY_NUM) 
            && ty_lhs->tag != TY_ERR 
            && ty_rhs->tag != TY_ERR) {
            push_errlist(&sema->errlist, node->base.span, "cannot apply operator");
            return mk_primitive_ty(&sema->scratch, TY_ERR);
        } else {
            // return ty_lhs because later we'll want to support adding lists and adding strings
            return ty_lhs;
        }
    case TOKEN_LT:
    case TOKEN_LEQ:
    case TOKEN_GT:
    case TOKEN_GEQ:
        if ((ty_lhs->tag != TY_NUM || ty_rhs->tag != TY_NUM) 
            && ty_lhs->tag != TY_ERR 
            && ty_rhs->tag != TY_ERR)
            push_errlist(&sema->errlist, node->base.span, "operands must be numbers");
        // even if the types were not Num, the inequality operator always returns bool
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    case TOKEN_NEQ:
    case TOKEN_EQ_EQ:
        if (!cmp_ty(ty_lhs, ty_rhs) 
            && ty_lhs->tag != TY_ERR 
            && ty_rhs->tag != TY_ERR)
            push_errlist(&sema->errlist, node->base.span, "operands must of be of same type");
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    case TOKEN_AND:
    case TOKEN_OR:
        if ((ty_lhs->tag != TY_BOOL || ty_rhs->tag != TY_BOOL) 
            && ty_lhs->tag != TY_ERR 
            && ty_rhs->tag != TY_ERR)
            push_errlist(&sema->errlist, node->base.span, "operands must be bools");
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    case TOKEN_EQ:
        if (!cmp_ty(ty_lhs, ty_rhs) 
            && ty_lhs->tag != TY_ERR 
            && ty_rhs->tag != TY_ERR) {
            push_errlist(&sema->errlist, node->base.span, "operands must be of same type");
            return mk_primitive_ty(&sema->scratch, TY_ERR);
        }
        return ty_lhs;
    }
}

static struct TyNode *analyze_block(struct SemaState *sema, struct BlockNode *node) {
    u32 count = sema->count;
    for (i32 i = 0; i < node->count; i++) {
        analyze_node(sema, node->stmts[i]);
    }
    sema->count = count;
    return mk_primitive_ty(&sema->scratch, TY_VOID);
}

static struct TyNode *analyze_if(struct SemaState *sema, struct IfNode *node) {
    struct TyNode *ty_cond = analyze_node(sema, node->cond);
    if (ty_cond->tag != TY_BOOL && ty_cond->tag != TY_ERR)
        push_errlist(&sema->errlist, node->base.span, "expected bool");

    analyze_block(sema, node->thn);
    if (node->els)
        analyze_block(sema, node->els);

    return mk_primitive_ty(&sema->scratch, TY_VOID);
}

static struct TyNode *analyze_expr_stmt(struct SemaState *sema, struct ExprStmtNode *node) {
    analyze_node(sema, node->expr);
    return mk_primitive_ty(&sema->scratch, TY_VOID);
}

static struct TyNode *analyze_print(struct SemaState *sema, struct PrintNode *node) {
    analyze_node(sema, node->expr);
    return mk_primitive_ty(&sema->scratch, TY_VOID);
}

static struct TyNode *analyze_var_decl(struct SemaState *sema, struct VarDeclNode *node) {
    // shadowing is allowed, even in the same scope. For example
    //      let x = 1;
    //      let x = "hello, world";
    u32 id = push_symtable(&sema->st, node->base.span);
    node->id = id;

    if (node->ty_hint)
        sema->st.symbols[id].ty = cpy_ty(&sema->st.arena, node->ty_hint);
    
    if (node->init) {
        sema->st.symbols[id].flags |= SYM_FLAG_INITIALIZED;
        struct TyNode *ty_init = analyze_node(sema, node->init);
        // if no type hint provided, infer type based on assignment
        if (!node->ty_hint) {
            sema->st.symbols[id].ty = cpy_ty(&sema->st.arena, ty_init);
        } else if (!cmp_ty(ty_init, node->ty_hint)) {
            // type hints take precedence over init type. For example
            //      let x: Bool = 3;
            //      x && false;
            // does not error, but 
            //      let x: Bool = 3;
            //      x + 4;
            // does error
            push_errlist(&sema->errlist, node->base.span, "value does not match type hint");
        }
    }
    
    // TODO error if more than 256 locals
    sema->locals[sema->count].id = id;
    sema->locals[sema->count].span = node->base.span;
    sema->count++;

    return mk_primitive_ty(&sema->scratch, TY_VOID);
}

static struct TyNode *analyze_node(struct SemaState *sema, struct Node *node) {
    switch (node->tag) {
    case NODE_LITERAL:
        return analyze_literal(sema, (struct LiteralNode*)node);
    case NODE_IDENT:
        return analyze_ident(sema, (struct IdentNode*)node);
    case NODE_UNARY:
        return analyze_unary(sema, (struct UnaryNode*)node);
    case NODE_BINARY:
        return analyze_binary(sema, (struct BinaryNode*)node);
    case NODE_BLOCK:
        return analyze_block(sema, (struct BlockNode*)node);
    case NODE_IF:
        return analyze_if(sema, (struct IfNode*)node);
    case NODE_EXPR_STMT:
        return analyze_expr_stmt(sema, (struct ExprStmtNode*)node);
    // TEMP remove when we add functions
    case NODE_PRINT:
        return analyze_print(sema, (struct PrintNode*)node);
    case NODE_VAR_DECL:
        return analyze_var_decl(sema, (struct VarDeclNode*)node);
    }
}

void analyze(struct SemaState *sema, struct Node *node) {
    analyze_node(sema, node);
    for (i32 i = 0; i < sema->st.count; i++) {
        if (!sema->st.symbols[i].ty)
            push_errlist(&sema->errlist, sema->st.symbols[i].span, "unable to infer type");
    }
}