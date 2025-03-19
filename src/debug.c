#include <stdio.h>
#include "debug.h"

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

void disassemble_chunk(struct Chunk *chunk) {
    for (u32 i = 0; i < chunk->count; i++) {
        switch (chunk->code[i]) {
            case OP_ADD:
                printf("OP_ADD\n");
                break;
            case OP_SUB:
                printf("OP_SUB\n");
                break;
            case OP_MUL:
                printf("OP_MUL\n");
                break;
            case OP_DIV:
                printf("OP_DIV\n");
                break;
            case OP_NEGATE:
                printf("OP_NEGATE\n");
                break;
            case OP_CONST:
                printf("OP_CONST");
                i++;
                printf("%16.4f\n", chunk->constants.arr[chunk->code[i]]);
                break;
            case OP_RETURN:
                printf("OP_RETURN\n");
        }
    }
}