#include <stdio.h>
#include "debug.h"

static void print_unary_epxr(struct UnaryExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(UnaryExpr\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_node(expr->rhs, offset + 2);
    printf(")");
}

static void print_binary_expr(struct BinaryExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(BinaryExpr\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_node(expr->lhs, offset + 2);
    printf("\n");
    print_node(expr->rhs, offset + 2);
    printf(")");
}

static void print_literal_expr(struct LiteralExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(LiteralExpr %.*s)", expr->value.length, expr->value.start);
}

static void print_block_expr(struct BlockExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(BlockExpr");
    struct NodeList* node_list = expr->node_list;
    while (node_list) {
        if (node_list->next) {
            printf("\n");
        }
        print_node(node_list->node, offset + 2);
        node_list = node_list->next;
    }
    printf(")");
}

static void print_if_expr(struct IfExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(IfExpr\n");
    print_node(expr->test, offset + 2);
    printf("\n");
    print_node((struct Node*) expr->true_block, offset + 2);
    if (expr->else_block) {
        printf("\n");
        print_node((struct Node*) expr->else_block, offset + 2);
    }
    printf(")");
}

static void print_expr_stmt(struct ExprStmt *stmt, u32 offset) {
    printf("%*s", offset, "");
    printf("(ExprStmt\n");
    print_node(stmt->expr, offset + 2);
    printf(")");
}

void print_node(struct Node *node, u32 offset) {
    if (!node) {
        return;
    }
    switch (node->type) {
        case EXPR_LITERAL:
            print_literal_expr((struct LiteralExpr*) node, offset);
            break;
        case EXPR_UNARY:
            print_unary_epxr((struct UnaryExpr*) node, offset);
            break;
        case EXPR_BINARY:
            print_binary_expr((struct BinaryExpr*) node, offset);
            break;
        case EXPR_BLOCK:
            print_block_expr((struct BlockExpr*) node, offset);
            break;
        case EXPR_IF:
            print_if_expr((struct IfExpr*) node, offset);
            break;
        case STMT_EXPR:
            print_expr_stmt((struct ExprStmt*) node, offset);
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