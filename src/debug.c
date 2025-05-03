#include <stdio.h>
#include "debug.h"

static void print_unary(struct UnaryNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Unary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", node->base.span.length, node->base.span.start);
    print_node(node->rhs, offset + 2);
    printf(")");
}

static void print_binary(struct BinaryNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Binary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", node->base.span.length, node->base.span.start);
    print_node(node->lhs, offset + 2);
    printf("\n");
    print_node(node->rhs, offset + 2);
    printf(")");
}

static void print_literal(struct LiteralNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Literal %.*s)", node->base.span.length, node->base.span.start);
}

static void print_ident(struct IdentNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Ident %.*s)", node->base.span.length, node->base.span.start);
}

static void print_block(struct BlockNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Block");
    for (i32 i = 0; i < node->count; i++) {
        printf("\n");
        print_node(node->stmts[i], offset + 2);
    }
    printf(")");
}

static void print_if(struct IfNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(If\n");
    print_node(node->cond, offset + 2);
    printf("\n");
    print_node((struct Node*)node->thn, offset + 2);
    if (node->els) {
        printf("\n");
        print_node((struct Node*)node->els, offset + 2);
    }
    printf(")");
}

static void print_expr_stmt(struct ExprStmtNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(ExprStmt\n");
    print_node(node->expr, offset + 2);
    printf(")");
}

static void print_var_decl(struct VarDeclNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(VarDecl\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.length, node->base.span.start);
    if (node->init) {
        printf("\n");
        print_node(node->init, offset + 2);
    }
    printf(")");
}

void print_node(struct Node *node, u32 offset) {
    switch (node->tag) {
        case NODE_LITERAL:
            print_literal((struct LiteralNode*)node, offset);
            break;
        case NODE_IDENT:
            print_ident((struct IdentNode*)node, offset);
            break;
        case NODE_UNARY:
            print_unary((struct UnaryNode*)node, offset);
            break;
        case NODE_BINARY:
            print_binary((struct BinaryNode*)node, offset);
            break;
        case NODE_BLOCK:
            print_block((struct BlockNode*)node, offset);
            break;
        case NODE_IF:
            print_if((struct IfNode*)node, offset);
            break;
        case NODE_EXPR_STMT:
            print_expr_stmt((struct ExprStmtNode*)node, offset);
            break;
        case NODE_VAR_DECL:
            print_var_decl((struct VarDeclNode*)node, offset);
            break;
    }
    if (offset == 0)
        printf("\n");
}