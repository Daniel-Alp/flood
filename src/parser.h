#pragma once
#include "../util/arena.h"
#include "scanner.h"

enum NodeType {
    EXPR_UNARY, EXPR_BINARY, EXPR_LITERAL, EXPR_IF, EXPR_BLOCK, STMT_EXPR
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

struct Parser {
    struct Scanner scanner;
    struct Token current;
    struct Token next;
    bool had_error;
};

bool matching_braces(struct Scanner *scanner);

struct BlockExpr *parse(struct Arena *arena, const char *source);