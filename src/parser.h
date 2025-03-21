#pragma once
#include "../util/arena.h"
#include "scanner.h"

enum NodeType {
    EXPR_UNARY, EXPR_BINARY, EXPR_LITERAL
};

struct Node {
    enum NodeType type;
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

struct LiteralExpr {
    struct Node base;
    struct Token value;
};

struct Parser {
    struct Scanner scanner;
    struct Token current;
    struct Token next;
    bool had_error;
    bool panic;
};

struct Node *parse(struct Arena *arena, const char *source);