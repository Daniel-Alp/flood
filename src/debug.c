#include <stdio.h>
#include "debug.h"

static void print_unary(struct UnaryNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Unary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", node->op.length, node->op.start);
    print_node(node->rhs, offset + 2);
    printf(")");
}

static void print_binary(struct BinaryNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Binary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s\n", node->op.length, node->op.start);
    print_node(node->lhs, offset + 2);
    printf("\n");
    print_node(node->rhs, offset + 2);
    printf(")");
}

static void print_literal(struct LiteralNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Literal %.*s)", node->val.length, node->val.start);
}

static void print_variable(struct VariableNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Variable %.*s id: %d)", node->name.length, node->name.start, node->id);
}

static void print_block(struct BlockNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Block");
    struct NodeList* stmts = node->stmts;
    while (stmts) {
        printf("\n");
        print_node(stmts->node, offset + 2);
        stmts = stmts->next;
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

static void print_var(struct VarDeclNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(VarDecl\n");
    print_variable(node->var, offset+2);
    if (node->init) {
        printf("\n");
        print_node(node->init, offset + 2);
    }
    printf(")");
}

void print_node(struct Node *node, u32 offset) {
    switch (node->type) {
        case NODE_LITERAL:
            print_literal((struct LiteralNode*)node, offset);
            break;
        case NODE_VARIABLE:
            print_variable((struct VariableNode*)node, offset);
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
            print_var((struct VarDeclNode*)node, offset);
            break;
    }
    if (!offset)
        printf("\n");
}