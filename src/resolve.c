#include <string.h>
#include "../util/memory.h"
#include "error.h"

// Name resolution associates every variable in the program with an id
// For example if our program was 
// 
//      var x = 40;
//      var y = 50;
//      {
//          var x = 60;
//          x = x + y;
//      }
// 
// Then after name resolution we would have
// 
//      var $0 = 40;
//      var $1 = 50;
//      {
//          var $2 = 60;
//          $2 = $2 + $1;
//      }
// 
// The symbol table is a map from these ids to a description of the variable, 
// which is used in later stages (typechecking, bytecode generation)

void init_symtable(struct SymTable *st) {
    st->count = 0;
    st->cap = 8;
    st->symbols = allocate(st->cap * sizeof(struct Symbol));
}

void free_symtable(struct SymTable *st) {
    for (i32 i = 0; i < st->count; i++)
        free_ty(&st->symbols[i].ty);
    release(st->symbols);
    st->symbols = NULL;
    st->count = 0;
    st->cap = 0;
}

static u32 push_symtable(struct SymTable *st, struct Symbol sym) {
    if (st->count == st->cap) {
        st->cap *= 2;
        st->symbols = reallocate(st->symbols, st->cap * sizeof(struct Symbol));
    }
    st->symbols[st->count] = sym;
    st->count++;
    return st->count-1;
}

static struct Local *resolve_local(struct Resolver *resolver, struct Span span) {
    for (i32 i = resolver->count-1; i >= 0; i--) {
        struct Local *local = &resolver->locals[i];
        // TODO speedup comparison
        if (span.length == local->span.length && memcmp(span.start, local->span.start, span.length) == 0)
            return local;
    }
    return NULL;
}

static void visit_node(struct SymTable *st, struct Node *node, struct Resolver *resolver);
static void visit_block(struct SymTable *st, struct BlockNode *node, struct Resolver *resolver);

static void visit_variable(struct SymTable *st, struct VariableNode *node, struct Resolver *resolver) {
    struct Local *local = resolve_local(resolver, node->base.span);
    if (local) {
        node->id = local->id;
    } else {
        node->id = -1;
        emit_resolver_error(node->base.span, "unbound identifier", resolver);
    }
}

static void visit_unary(struct SymTable *st, struct UnaryNode *node, struct Resolver *resolver) {
    visit_node(st, node->rhs, resolver);
}

static void visit_binary(struct SymTable *st, struct BinaryNode *node, struct Resolver *resolver) {
    visit_node(st, node->lhs, resolver);
    visit_node(st, node->rhs, resolver);
}

static void visit_if(struct SymTable *st, struct IfNode *node, struct Resolver *resolver) {
    visit_node(st, node->cond, resolver);
    visit_block(st, node->thn, resolver);
    if (node->els)
        visit_block(st, node->els, resolver);
}

static void visit_block(struct SymTable *st, struct BlockNode *node, struct Resolver *resolver) {
    resolver->depth++;
    struct NodeLinkedList *stmts = node->stmts;
    while (stmts) {
        visit_node(st, stmts->node, resolver);
        stmts = stmts->next;
    }
    resolver->depth--;
    while (resolver->count-1 >= 0) {
        if (resolver->locals[resolver->count-1].depth == resolver->depth)
            break;
        resolver->count--;
    }
}

static void visit_var_decl(struct SymTable *st, struct VarDeclNode *node, struct Resolver *resolver) {
    if (node->init)
        visit_node(st, node->init, resolver);

    struct Span var_span = node->var->base.span;
    struct Local *local = resolve_local(resolver, var_span);
    if (local && local->depth == resolver->depth) {
        emit_resolver_error(var_span, "redeclared variable", resolver);
        return;
    }

    struct Symbol sym = {.span = var_span};
    init_ty(&sym.ty);
    push_ty(&sym.ty, TY_ANY);
    u32 id = push_symtable(st, sym);
    node->var->id = id;

    // TODO error if more than 256 locals in scope
    struct Local new = {
        .depth = resolver->depth,
        .id = id,
        .span = var_span
    };
    resolver->locals[resolver->count] = new;
    resolver->count++;
}

static void visit_node(struct SymTable *st, struct Node *node, struct Resolver *resolver) {
    switch (node->kind) {
        case NODE_LITERAL:
            return;
        case NODE_VARIABLE:
            visit_variable(st, (struct VariableNode*)node, resolver);
            break;
        case NODE_UNARY:
            visit_unary(st, (struct UnaryNode*)node, resolver);
            break;
        case NODE_BINARY:
            visit_binary(st, (struct BinaryNode*)node, resolver);
            break;
        case NODE_IF:
            visit_if(st, (struct IfNode*)node, resolver);
            break;
        case NODE_BLOCK:
            visit_block(st, (struct BlockNode*)node, resolver);
            break;
        case NODE_EXPR_STMT:
            visit_node(st, ((struct ExprStmtNode*)node)->expr, resolver);
            break;
        case NODE_VAR_DECL:
            visit_var_decl(st, (struct VarDeclNode*)node, resolver);
            break;
    }
}

bool resolve_names(struct SymTable *st, struct Node *node) {
    struct Resolver resolver = {
        .count = 0,
        .depth = 0,
        .had_error = false
    };
    visit_node(st, node, &resolver);
    return !resolver.had_error;
}