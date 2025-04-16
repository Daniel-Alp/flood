#pragma once
#include "../util/arena.h"
#include "scanner.h"

enum NodeType {
    EXPR_UNARY, 
    EXPR_BINARY, 
    EXPR_LITERAL, 
    EXPR_IF, 
    EXPR_BLOCK, 
    STMT_EXPR, 
    STMT_VAR
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
    struct NodeList *stmts;
};

struct IfExpr {
    struct Node base;
    struct Node *cond;
    struct BlockExpr *thn;
    struct BlockExpr *els;
};

struct ExprStmt {
    struct Node base;
    struct Node *expr;
};

struct VarStmt {
    struct Node base;
    struct Token id;
    struct Node *init;
};

struct Parser {
    struct Scanner scanner;
    struct Token at;
    bool had_error;
};

struct BlockExpr *parse(struct Arena *arena, const char *source);