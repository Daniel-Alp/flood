#include <string.h>
#include "../util/memory.h"
#include "error.h"
#include "ty.h"

void init_ty(struct Ty *ty) {
    ty->count = 0;
    ty->cap = 8;
    ty->arr = allocate(ty->cap * sizeof(u32));
}

void free_ty(struct Ty *ty) {
    ty->count = 0;
    ty->cap = 0;
    release(ty->arr);
    ty->arr = NULL;
}

struct Ty mk_primitive(enum TyKind primitive) {
    struct Ty ty;
    init_ty(&ty);
    push_ty(&ty, primitive);
    return ty;
}

void push_ty(struct Ty *ty, u32 val) {
    if (ty->count == ty->cap) {
        ty->cap *= 2;
        ty->arr = reallocate(ty->arr, ty->cap * sizeof(u32));
    }
    ty->arr[ty->count] = val;
    ty->count++;
}

// e.g given types Num and List[Num] we can create the type Map[Num, List[Num]] by doing 
// 
//      append_ty(map_ty, list_ty)
//      append_ty(map_ty, num_ty)
//      push_ty(map_ty, TY_MAP)
// 
void append_ty(struct Ty *ty_tgt, struct Ty *ty_src) {
    for (i32 i = 0; i < ty_src->count; i++)
        push_ty(ty_tgt, ty_src->arr[i]);
}

void cpy_ty(struct Ty *ty_tgt, struct Ty *ty_src) {
    ty_tgt->count = ty_src->count;
    ty_tgt->cap = ty_src->cap;
    ty_tgt->arr = reallocate(ty_tgt->arr, ty_tgt->cap * sizeof(u32));
    memcpy(ty_tgt->arr, ty_src->arr, ty_tgt->count * sizeof(u32));
}

bool cmp_ty(struct Ty *ty1, struct Ty* ty2) {
    if (ty1->count != ty2->count)
        return false;
    for (i32 i = 0; i < ty1->count; i++) {
        if (ty1->arr[i] != ty2->arr[i])
            return false;
    }
    return true;
}

// e.g. given the type List[Num] the head is TY_LIST
u32 ty_head(struct Ty *ty) {
    return ty->arr[ty->count-1];
}

static struct Ty visit_node(struct SymTable *st, struct Node *node, bool *had_error);
static struct Ty visit_block(struct SymTable *st, struct BlockNode *node, bool *had_error);

static struct Ty visit_literal(struct SymTable *st, struct LiteralNode *node, bool *had_error) {
    switch(node->kind) {
        case LIT_NUMBER:
            return mk_primitive(TY_NUM);
        case LIT_FALSE:
        case LIT_TRUE:
            return mk_primitive(TY_BOOL);
    }
}

// assumes that the variable exists in the symbol table
static struct Ty visit_variable(struct SymTable *st, struct VariableNode *node, bool *had_error) {
    struct Ty *ty_lookup = &st->symbols[node->id].ty;
    // set the type of the variable to be ERR, this prevents unintialized variable error from being emitted later
    if (ty_head(ty_lookup) == TY_ANY) {
        emit_ty_error_uninitialized(node->base.span, had_error);
        struct Ty ty_err = mk_primitive(TY_ERR);
        cpy_ty(ty_lookup, &ty_err);
    }
    struct Ty ty;
    init_ty(&ty);
    cpy_ty(&ty, ty_lookup);
    return ty;
}

static struct Ty visit_unary(struct SymTable *st, struct UnaryNode *node, bool *had_error) {
    struct Ty ty = visit_node(st, node->rhs, had_error);
    if ((node->op.kind == UNOP_NEG && ty_head(&ty) == TY_NUM)
        || (node->op.kind == UNOP_NOT && ty_head(&ty) == TY_BOOL))
        return ty;
    emit_ty_error_unary(node->op.span, &ty, had_error);
    free_ty(&ty);
    return mk_primitive(TY_ERR);
}

static struct Ty visit_binary(struct SymTable *st, struct BinaryNode *node, bool *had_error) {
    struct Ty ty_lhs;
    init_ty(&ty_lhs);
    // lookup ty_lhs after because if lhs is a variable that is currently uninitialized we infer its type
    struct Ty ty_rhs = visit_node(st, node->rhs, had_error);
    if (node->op.kind == BINOP_EQ) {
        if (node->lhs->kind != NODE_VARIABLE) {
            emit_ty_error_invalid_assignment(node->lhs->span, had_error);
            free_ty(&ty_rhs);
            return mk_primitive(TY_ERR);
        }
        struct Ty *ty_lookup = &st->symbols[((struct VariableNode*)node->lhs)->id].ty;
        // if rhs is TY_ANY the error is caught below
        if (ty_head(ty_lookup) == TY_ANY) {
            cpy_ty(ty_lookup, &ty_rhs);
        }
        cpy_ty(&ty_lhs, ty_lookup);
    } else {
        ty_lhs = visit_node(st, node->lhs, had_error);
    }
    switch(node->op.kind) {
        case BINOP_ADD:
        case BINOP_SUB:
        case BINOP_MUL:
        case BINOP_DIV:
        if (ty_head(&ty_lhs) == TY_NUM && ty_head(&ty_rhs) == TY_NUM) {
            free_ty(&ty_rhs);
            return ty_lhs;
        } else {
            goto err;
        }
        case BINOP_LT:
        case BINOP_GT:
        case BINOP_LEQ:
        case BINOP_GEQ:
        if (ty_head(&ty_lhs) == TY_NUM && ty_head(&ty_rhs) == TY_NUM) {
            free_ty(&ty_lhs);
            free_ty(&ty_rhs);
            return mk_primitive(TY_BOOL);
        } else {
            goto err;
        }
        case BINOP_EQEQ:
        if (cmp_ty(&ty_lhs, &ty_rhs) && ty_head(&ty_lhs) != TY_ANY && ty_head(&ty_lhs) != TY_ERR) {
            free_ty(&ty_lhs);
            free_ty(&ty_rhs);
            return mk_primitive(TY_BOOL);
        } else {
            goto err;
        }
        case BINOP_AND:
        case BINOP_OR:
        if (ty_head(&ty_lhs) == TY_BOOL && ty_head(&ty_rhs) == TY_BOOL) {
            free_ty(&ty_rhs);
            return ty_lhs;
        } else {
            goto err;
        }
        case BINOP_EQ:
        if (cmp_ty(&ty_lhs, &ty_rhs) && ty_head(&ty_lhs) != TY_ANY && ty_head(&ty_lhs) != TY_ERR) {
            free_ty(&ty_rhs);
            return ty_lhs;
        } else {
            goto err;            
        }
    }
err:
    emit_ty_error_binary(node->op.span, &ty_lhs, &ty_rhs, had_error);
    free_ty(&ty_lhs);
    free_ty(&ty_rhs);
    return mk_primitive(TY_ERR);
}

static struct Ty visit_if(struct SymTable *st, struct IfNode *node, bool *had_error) {
    struct Ty ty_cond = visit_node(st, node->cond, had_error);
    if (ty_head(&ty_cond) != TY_BOOL)
        emit_ty_error_if_cond(node->cond->span, &ty_cond, had_error);
    free_ty(&ty_cond);

    struct Ty ty_thn = visit_block(st, node->thn, had_error);
    struct Ty ty_els;
    if (node->els)
        ty_els = visit_block(st, node->els, had_error);
    else
        ty_els = mk_primitive(TY_UNIT);
    
    if (cmp_ty(&ty_thn, &ty_els) && ty_head(&ty_thn) != TY_ANY && ty_head(&ty_thn) != TY_ERR) {
        free_ty(&ty_els);
        return ty_thn;
    }
    emit_ty_error_if(node->base.span, &ty_thn, &ty_els, had_error);
    free_ty(&ty_thn);
    free_ty(&ty_els);
    return mk_primitive(TY_ERR);
}

static struct Ty visit_block(struct SymTable *st, struct BlockNode *node, bool *had_error) {
    struct NodeLinkedList *stmts = node->stmts;
    if (!stmts)
        return mk_primitive(TY_UNIT);
    while (true) {
        struct Ty ty = visit_node(st, stmts->node, had_error);
        if (!stmts->next) 
            return ty;
        free_ty(&ty);
        stmts = stmts->next;
    }
}

static struct Ty visit_var_decl(struct SymTable *st, struct VarDeclNode *node, bool *had_error) {
    if (node->init) {
        struct Ty ty = visit_node(st, node->init, had_error);
        cpy_ty(&st->symbols[node->var->id].ty, &ty);
        free_ty(&ty);
    }
    return mk_primitive(TY_UNIT);
}

static struct Ty visit_node(struct SymTable *st, struct Node *node, bool *had_error) {
    switch(node->kind) {
        case NODE_LITERAL:
            return visit_literal(st, (struct LiteralNode*)node, had_error);
        case NODE_VARIABLE:
            return visit_variable(st, (struct VariableNode*)node, had_error);
        case NODE_UNARY:
            return visit_unary(st, (struct UnaryNode*)node, had_error);
        case NODE_BINARY:
            return visit_binary(st, (struct BinaryNode*)node, had_error);
        case NODE_IF:
            return visit_if(st, (struct IfNode*)node, had_error);
        case NODE_BLOCK:
            return visit_block(st, (struct BlockNode*)node, had_error);
        case NODE_EXPR_STMT:
            struct Ty ty = visit_node(st, ((struct ExprStmtNode*)node)->expr, had_error);
            free_ty(&ty);
            return mk_primitive(TY_UNIT);
        case NODE_VAR_DECL:
            return visit_var_decl(st, (struct VarDeclNode*)node, had_error);
    }
}

bool resolve_tys(struct SymTable *st, struct Node *node) {
    bool had_error = false;
    struct Ty ty = visit_node(st, node, &had_error);
    free_ty(&ty);
    for (i32 i = 0; i < st->count; i++) {
        if (ty_head(&st->symbols[i].ty) == TY_ANY) {
            emit_ty_error_cannot_infer(st->symbols[i].span);
            had_error = true;
        }
    }
    return had_error;
}