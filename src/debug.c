#include <stdio.h>
#include "debug.h"

void print_expr(struct Expr *expr, u32 offset);

static void print_unary_epxr(struct UnaryExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(UnaryExpr\n");
    printf("%*s", offset + 4, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_expr(expr->rhs, offset + 4);
    printf(")");
}

static void print_binary_expr(struct BinaryExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(BinaryExpr\n");
    printf("%*s", offset + 4, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_expr(expr->lhs, offset + 4);
    printf("\n");
    print_expr(expr->rhs, offset + 4);
    printf(")");
}

static void print_literal_expr(struct LiteralExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(LiteralExpr %.*s)", expr->value.length, expr->value.start);
}

void print_expr(struct Expr *expr, u32 offset) {
    if (!expr) {
        return;
    }
    switch (expr->type) {
        case EXPR_LITERAL:
            print_literal_expr((struct LiteralExpr*) expr, offset);
            break;
        case EXPR_UNARY:
            print_unary_epxr((struct UnaryExpr*) expr, offset);
            break;
        case EXPR_BINARY:
            print_binary_expr((struct BinaryExpr*) expr, offset);
            break;
    }
}