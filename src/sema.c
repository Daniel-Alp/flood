#include <stdio.h> //!DELETEME
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
    st->symbols[st->count].idx = -1;
    st->count++;
    return st->count-1;
}

void init_sema_state(struct SemaState *sema) {
    sema->local_count = 0;
    sema->global_count = 0;
    sema->enclosing_fn_idx = -1;
    sema->depth = 0;
    init_errlist(&sema->errlist);
    init_arena(&sema->scratch);
    init_symtable(&sema->st);
}

void release_sema_state(struct SemaState *sema) {
    sema->local_count = 0;
    release_errlist(&sema->errlist);
    release_arena(&sema->scratch);
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
    struct Ident *ident = resolve_ident(sema, node->base.span);
    // TODO only error on first occurence of not in scope
    if (!ident) {
        push_errlist(&sema->errlist, node->base.span, "not found in this scope");
        return mk_primitive_ty(&sema->scratch, TY_ANY);
    }
    struct Symbol symbol = sema->st.symbols[ident->id];
    if (!symbol.ty) {
        push_errlist(&sema->errlist, node->base.span, "unknown type");
        return mk_primitive_ty(&sema->scratch, TY_ANY);
    }
    if ((symbol.flags & SYM_FLAG_INITIALIZED) == 0) {
        push_errlist(&sema->errlist, node->base.span, "used before initialization");
        return mk_primitive_ty(&sema->scratch, TY_ANY);
    }
    node->id = ident->id;
    return symbol.ty;
}

static struct TyNode *analyze_unary(struct SemaState *sema, struct UnaryNode *node) {
    struct TyNode *ty = analyze_node(sema, node->rhs);
    if (((node->op_tag == TOKEN_MINUS && ty->tag != TY_NUM) || (node->op_tag == TOKEN_NOT && ty->tag != TY_BOOL))
        && ty->tag != TY_ANY) {
        push_errlist(&sema->errlist, node->base.span, "cannot apply operator");
        return mk_primitive_ty(&sema->scratch, TY_ANY);
    }
    return ty;
}

static struct TyNode *analyze_binary(struct SemaState *sema, struct BinaryNode *node) {
    if (node->op_tag == TOKEN_EQ && node->lhs->tag != NODE_IDENT) {
        push_errlist(&sema->errlist, node->base.span, "cannot assign to left-hand expression");
        return mk_primitive_ty(&sema->scratch, TY_ANY);
    }

    struct TyNode *ty_rhs = analyze_node(sema, node->rhs);
    struct TyNode *ty_lhs = NULL;
    // type inference. For example,
    //      var x;
    //      x = 5;
    if (node->op_tag == TOKEN_EQ && node->lhs->tag == NODE_IDENT) {
        struct Ident *loc = resolve_ident(sema, ((struct IdentNode*)node->lhs)->base.span);
        if (!loc) {
            push_errlist(&sema->errlist, ((struct IdentNode*)node->lhs)->base.span, "not found in this scope");
            return mk_primitive_ty(&sema->scratch, TY_ANY);
        }

        u32 id = loc->id;
        ((struct IdentNode*)node->lhs)->id = id;

        if (sema->st.symbols[id].flags & SYM_FLAG_IMMUTABLE) {
            push_errlist(&sema->errlist, ((struct IdentNode*)node->lhs)->base.span, "cannot reassign variable");
            return mk_primitive_ty(&sema->scratch, TY_ANY);
        }

        if (!sema->st.symbols[id].ty)
            sema->st.symbols[id].ty = cpy_ty(&sema->st.arena, ty_rhs); 
        
        ty_lhs = sema->st.symbols[id].ty;
        sema->st.symbols[id].flags |= SYM_FLAG_INITIALIZED;
    } else {
        ty_lhs = analyze_node(sema, node->lhs);
    }

    switch(node->op_tag) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
        if ((ty_lhs->tag != TY_NUM || ty_rhs->tag != TY_NUM) 
            && ty_lhs->tag != TY_ANY 
            && ty_rhs->tag != TY_ANY) {
            push_errlist(&sema->errlist, node->base.span, "operands must be numbers");
            return mk_primitive_ty(&sema->scratch, TY_ANY);
        } else {
            return ty_lhs;
        }
    case TOKEN_LT:
    case TOKEN_LEQ:
    case TOKEN_GT:
    case TOKEN_GEQ:
        if ((ty_lhs->tag != TY_NUM || ty_rhs->tag != TY_NUM) 
            && ty_lhs->tag != TY_ANY 
            && ty_rhs->tag != TY_ANY)
            push_errlist(&sema->errlist, node->base.span, "operands must be numbers");
        // even if the types were not Num, the inequality operator always returns bool
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    case TOKEN_NEQ:
    case TOKEN_EQ_EQ:
        if (!cmp_ty(ty_lhs, ty_rhs))
            push_errlist(&sema->errlist, node->base.span, "operands must of be of same type");
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    case TOKEN_AND:
    case TOKEN_OR:
        if ((ty_lhs->tag != TY_BOOL || ty_rhs->tag != TY_BOOL) 
            && ty_lhs->tag != TY_ANY 
            && ty_rhs->tag != TY_ANY)
            push_errlist(&sema->errlist, node->base.span, "operands must be bools");
        return mk_primitive_ty(&sema->scratch, TY_BOOL);
    case TOKEN_EQ:
        if (!cmp_ty(ty_lhs, ty_rhs)) {
            push_errlist(&sema->errlist, node->base.span, "operands must be of same type");
            return mk_primitive_ty(&sema->scratch, TY_ANY);
        }
        return ty_lhs;
    }
}

static struct TyNode *analyze_fn_call(struct SemaState *sema, struct FnCallNode *node) {
    struct TyNode *ty_lhs = analyze_node(sema, node->lhs);
    if (ty_lhs->tag != TY_FN && ty_lhs->tag != TY_ANY) {
        push_errlist(&sema->errlist, node->base.span, "left-hand expression is not callable");
        return mk_primitive_ty(&sema->scratch, TY_ANY);        
    }
    struct FnTyNode *ty_cast = (struct FnTyNode*)ty_lhs;
    if (ty_cast->arity != node->arity) {
        push_errlist(&sema->errlist, node->base.span, "incorrect number of arguments provided");
    }
    for (i32 i = 0; i < node->arity; i++) {
        struct TyNode *ty_arg = analyze_node(sema, node->args[i]);
        if (i < ty_cast->arity && !cmp_ty(ty_arg, ty_cast->param_tys[i]))
            push_errlist(&sema->errlist, node->arg_spans[i], "argument type does not match parameter type");
    }
    return ty_cast->ret_ty;
}

static struct TyNode *analyze_block(struct SemaState *sema, struct BlockNode *node) {
    u32 count = sema->local_count;
    if (node->count == 0)
        return mk_primitive_ty(&sema->scratch, TY_VOID);
    sema->depth++;
    struct TyNode *ty;
    for (i32 i = 0; i < node->count; i++)
        ty = analyze_node(sema, node->stmts[i]);
    sema->depth--;
    sema->local_count = count;
    return ty;
}

static struct TyNode *analyze_if(struct SemaState *sema, struct IfNode *node) {
    struct TyNode *ty_cond = analyze_node(sema, node->cond);
    if (ty_cond->tag != TY_BOOL && ty_cond->tag != TY_ANY)
        push_errlist(&sema->errlist, node->base.span, "expected bool");

    struct TyNode *ty_thn = analyze_block(sema, node->thn);
    if (node->els) {
        struct TyNode *ty_els = analyze_block(sema, node->els);
        if (ty_thn->tag == TY_NEVER && ty_els->tag == TY_NEVER)
            return mk_primitive_ty(&sema->scratch, TY_NEVER);
    }

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

static struct TyNode *analyze_return(struct SemaState *sema, struct ReturnNode *node) {
    // TODO if I allow top level statements (apart from function and variable declarations) 
    // do not allow a return statement at the top level
    struct TyNode *ty;
    if (node->expr)
        ty = analyze_node(sema, node->expr);
    else
        ty = mk_primitive_ty(&sema->scratch, TY_VOID);
    struct TyNode *ret_ty = ((struct FnTyNode*)sema->st.symbols[sema->enclosing_fn_idx].ty)->ret_ty;
    if (!cmp_ty(ty, ret_ty))
        push_errlist(&sema->errlist, node->base.span, "return value does not match function return type");
    return mk_primitive_ty(&sema->scratch, TY_NEVER);
}

static struct TyNode *analyze_var_decl(struct SemaState *sema, struct VarDeclNode *node) {
    struct Ident *ident = resolve_ident(sema, node->base.span);
    if (ident && ident->depth == sema->depth) {
        push_errlist(&sema->errlist, node->base.span, "redeclared variable");
        return mk_primitive_ty(&sema->scratch, TY_VOID);
    }

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
            // type hints are prioritized over init type. For example
            //      let x: Bool = 3;
            //      x && false;
            // does not error on the second line, but 
            //      let x: Bool = 3;
            //      x + 4;
            // does error on the second line
            push_errlist(&sema->errlist, node->base.span, "value does not match type hint");
        }
    }

    if (sema->depth == 0)
        sema->st.symbols[id].flags |= SYM_FLAG_GLOBAL;
    else
        sema->st.symbols[id].flags |= SYM_FLAG_LOCAL;
    
    // TODO variables (currently only function declarations at the top level)
    // TODO error if more than 256 locals
    sema->locals[sema->local_count].id = id;
    sema->locals[sema->local_count].span = node->base.span;
    sema->locals[sema->local_count].depth = sema->depth;
    sema->local_count++;
    return mk_primitive_ty(&sema->scratch, TY_VOID);
}

// function declarations
//      fn foo(x: Num, y: Num) {
//          ...
//      }
// are only allowed at the top level. They cannot be assigned to, (the following is not allowed)
//      foo = bar; 
// even if bar is a function with the same signature as foo
// but functions can be passed as parameters or assigned to variables. E.g. 
//      var x = foo; 
//      baz(foo);
static void analyze_fn_decl(struct SemaState *sema, struct FnDeclNode *node) {
    // TODO error if more than 256 globals
    u32 id = push_symtable(&sema->st, node->base.span);
    node->id = id;

    sema->st.symbols[id].ty = node->ty;
    sema->st.symbols[id].flags |= SYM_FLAG_GLOBAL | SYM_FLAG_INITIALIZED | SYM_FLAG_IMMUTABLE;

    sema->globals[sema->global_count].id = id;
    sema->globals[sema->global_count].span = node->base.span;
    sema->globals[sema->global_count].depth = 0;
    sema->global_count++;
}

static struct TyNode *analyze_node(struct SemaState *sema, struct Node *node) {
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

void analyze(struct SemaState *sema, struct FileNode *file) {
    for (i32 i = 0; i < file->count; i++) 
        analyze_fn_decl(sema, file->stmts[i]);

    for (i32 i = 0; i < file->count; i++) {
        // TODO global variable declarations
        struct FnDeclNode *node = (struct FnDeclNode *)file->stmts[i];

        // sema->local_count = 0 because this is at the top level
        for (i32 i = 0; i < node->ty->arity; i++) {
            struct Span span = node->ty->param_spans[i];
            for (i32 j = 0; j < i; j++) {
                struct Span other_span = node->ty->param_spans[j];
                if (other_span.length == span.length && memcmp(other_span.start, span.start, span.length) == 0) {
                    push_errlist(&sema->errlist, span, "used as parameter more than once");
                    break;
                }
            }
            u32 id = push_symtable(&sema->st, node->base.span);
            // TODO error if more than 256 locals
            sema->st.symbols[id].ty = node->ty->param_tys[i];
            sema->st.symbols[id].flags |= SYM_FLAG_INITIALIZED;
            sema->locals[i].id = id;
            sema->locals[i].depth = 1;
            sema->locals[i].span = span;
        }
        sema->local_count = node->ty->arity;
        sema->enclosing_fn_idx = node->id;
        struct TyNode *ty = analyze_block(sema, node->body);
        if (node->ty->ret_ty->tag != TY_VOID && ty->tag != TY_NEVER)
            push_errlist(&sema->errlist, node->base.span, "missing return statement"); 
    }

    for (i32 i = 0; i < sema->st.count; i++) {
        if (!sema->st.symbols[i].ty)
            push_errlist(&sema->errlist, sema->st.symbols[i].span, "add type hint");
    }
}