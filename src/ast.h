#pragma once
#include "../util/arena.h"
#include "scanner.h"

enum NodeType {
    EXPR_UNARY, EXPR_BINARY, EXPR_LITERAL, EXPR_IF, EXPR_BLOCK, STMT_EXPR, STMT_VAR
};

struct Node {
    enum NodeType type;
};

struct LiteralExpr {
    struct Node base;
    struct Token val;
};

struct UnaryExpr {
    struct Node base;
    struct Node *rhs;
    struct Token op;
};

struct BinaryExpr {
    struct Node base;
    struct Node *lhs;
    struct Node *rhs;
    struct Token op;
};

struct NodeList {
    struct Node *node;
    struct NodeList *next;
};

struct BlockExpr {
    struct Node base;
    struct NodeList *node_list;
};

struct IfExpr {
    struct Node base;
    struct Node* test;
    struct BlockExpr *true_block;
    struct BlockExpr *else_block;
};

struct ExprStmt {
    struct Node base;
    struct Node *expr;
};

struct VarStmt {
    struct Node base;
    struct Token id;
    struct Node *expr;
};

struct LiteralExpr *make_literal_expr(struct Arena *arena, struct Token val);

struct UnaryExpr *make_unary_expr(struct Arena *arena, struct Node *rhs, struct Token op);

struct BinaryExpr *make_binary_expr(struct Arena *arena, struct Node *lhs, struct Node *rhs, struct Token op);

struct IfExpr *make_if_expr(struct Arena *arena, struct Node *test, struct BlockExpr *if_block, struct BlockExpr *else_block);

struct BlockExpr *make_block_expr(struct Arena *arena, struct NodeList *node_list);

struct ExprStmt *make_expr_stmt(struct Arena *arena, struct Node *expr);

struct VarStmt *make_var_stmt(struct Arena *arena, struct Token id, struct Node *expr);