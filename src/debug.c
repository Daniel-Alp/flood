#include <stdio.h>
#include "debug.h"

static void print_unary(struct UnaryExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(Unary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_node(expr->rhs, offset + 2);
    printf(")");
}

static void print_binary(struct BinaryExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(Binary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", expr->op.length, expr->op.start);
    print_node(expr->lhs, offset + 2);
    printf("\n");
    print_node(expr->rhs, offset + 2);
    printf(")");
}

static void print_literal(struct LiteralExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(Literal %.*s)", expr->val.length, expr->val.start);
}

static void print_block(struct BlockExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(Block");
    struct NodeList* stmts = expr->stmts;
    while (stmts) {
        if (stmts->next)
            printf("\n");
        print_node(stmts->node, offset + 2);
        stmts = stmts->next;
    }
    printf(")");
}

static void print_if(struct IfExpr *expr, u32 offset) {
    printf("%*s", offset, "");
    printf("(If\n");
    print_node(expr->cond, offset + 2);
    printf("\n");
    print_node((struct Node*) expr->thn, offset + 2);
    if (expr->els) {
        printf("\n");
        print_node((struct Node*) expr->els, offset + 2);
    }
    printf(")");
}

static void print_expr_stmt(struct ExprStmt *stmt, u32 offset) {
    printf("%*s", offset, "");
    printf("(ExprStmt\n");
    print_node(stmt->expr, offset + 2);
    printf(")");
}

static void print_var(struct VarStmt *stmt, u32 offset) {
    printf("%*s", offset, "");
    printf("(Var\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", stmt->id.length, stmt->id.start);
    if (stmt->init) {
        printf("\n");
        print_node(stmt->init, offset + 2);
    }
    printf(")");
}

void print_node(struct Node *node, u32 offset) {
    if (!node)
        return;
    switch (node->type) {
        case EXPR_LITERAL:
            print_literal((struct LiteralExpr*) node, offset);
            break;
        case EXPR_UNARY:
            print_unary((struct UnaryExpr*) node, offset);
            break;
        case EXPR_BINARY:
            print_binary((struct BinaryExpr*) node, offset);
            break;
        case EXPR_BLOCK:
            print_block((struct BlockExpr*) node, offset);
            break;
        case EXPR_IF:
            print_if((struct IfExpr*) node, offset);
            break;
        case STMT_EXPR:
            print_expr_stmt((struct ExprStmt*) node, offset);
            break;
        case STMT_VAR:
            print_var((struct VarStmt*) node, offset);
            break;
    }
    if (!offset)
        printf("\n");
}