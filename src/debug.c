#include <stdio.h>
#include "debug.h"

void print_expr(struct Expr *expr, u32 indent);

static void print_binary_expr(struct BinaryExpr *expr, u32 indent) {
    printf("%*s", indent, "");
    printf("(BinaryExpr\n");
    printf("%*s", indent + 4, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_expr(expr->lhs, indent + 4);
    printf("\n");
    print_expr(expr->rhs, indent + 4);
    printf(")");
}

static void print_literal_expr(struct LiteralExpr *expr, u32 indent) {
    printf("%*s", indent, "");
    printf("(LiteralExpr %.*s)", expr->value.length, expr->value.start);
}

void print_expr(struct Expr *expr, u32 indent) {
    if (!expr) {
        return;
    }
    switch (expr->type) {
        case EXPR_BINARY:
            print_binary_expr((struct BinaryExpr*) expr, indent);
            break;
        case EXPR_LITERAL:
            print_literal_expr((struct LiteralExpr*) expr, indent);
            break;
    }
}