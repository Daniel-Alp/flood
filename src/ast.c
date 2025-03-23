#include "ast.h"

struct LiteralExpr *make_literal_expr(struct Arena *arena, struct Token val) {
    struct LiteralExpr *expr = arena_push(arena, sizeof(struct LiteralExpr));
    expr->base.type = EXPR_LITERAL;
    expr->val = val;
    return expr;
}

struct UnaryExpr *make_unary_expr(struct Arena *arena, struct Node *rhs, struct Token op) {
    struct UnaryExpr *expr = arena_push(arena, sizeof(struct UnaryExpr));
    expr->base.type = EXPR_UNARY;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

struct BinaryExpr *make_binary_expr(struct Arena *arena, struct Node *lhs, struct Node *rhs, struct Token op) {
    struct BinaryExpr *expr = arena_push(arena, sizeof(struct BinaryExpr));
    expr->base.type = EXPR_BINARY;
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

struct IfExpr *make_if_expr(struct Arena *arena, struct Node *test, struct BlockExpr *if_block, struct BlockExpr *else_block) {
    struct IfExpr *expr = arena_push(arena, sizeof(struct IfExpr));
    expr->base.type = EXPR_IF;
    expr->test = test;
    expr->true_block = if_block;
    expr->else_block = else_block;
    return expr;
}

struct BlockExpr *make_block_expr(struct Arena *arena, struct NodeList *node_list) {
    struct BlockExpr *expr = arena_push(arena, sizeof(struct BlockExpr));
    expr->base.type = EXPR_BLOCK;
    expr->node_list = node_list;
    return expr;
}

struct ExprStmt *make_expr_stmt(struct Arena *arena, struct Node *expr) {
    struct ExprStmt *stmt = arena_push(arena, sizeof(struct ExprStmt));
    stmt->base.type = STMT_EXPR;
    stmt->expr = expr;
    return stmt;
}

struct VarStmt *make_var_stmt(struct Arena *arena, struct Token id, struct Node *expr) {
    struct VarStmt *stmt = arena_push(arena, sizeof(struct VarStmt));
    stmt->base.type = STMT_VAR;
    stmt->id = id;
    stmt->expr = expr;
    return stmt;
}