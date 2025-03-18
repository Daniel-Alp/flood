#pragma once
#include "../util/arena.h"
#include "scanner.h"

enum ExprType {
    EXPR_UNARY, EXPR_BINARY, EXPR_LITERAL
};

struct Expr {
    enum ExprType type;
};

struct UnaryExpr {
    struct Expr base;
    struct Expr *rhs;
    struct Token op;
};

struct BinaryExpr {
    struct Expr base;
    struct Expr *lhs;
    struct Expr *rhs;
    struct Token op;
};

struct LiteralExpr {
    struct Expr base;
    struct Token value;
};

struct Parser {
    struct Scanner scanner;
    struct Token current;
    struct Token next;
    bool had_error;
    bool panic;
};

struct Expr *parse(struct Arena *arena, const char *source);