#include <stdio.h>
#include "debug.h"

// we cannot use the token span for printing the binary operator because  
//      x += 3;
// is desugared into
//      x = x + 3;
const char *bin_op_to_str(enum TokenTag op_tag) {
    switch(op_tag) {
    case TOKEN_PLUS:  return "+";
    case TOKEN_MINUS: return "-";
    case TOKEN_STAR:  return "*";
    case TOKEN_SLASH: return "/";
    case TOKEN_LT:    return "<";
    case TOKEN_LEQ:   return "<=";
    case TOKEN_GT:    return ">";
    case TOKEN_GEQ:   return ">=";
    case TOKEN_EQ_EQ: return "==";
    case TOKEN_NEQ:   return "!=";
    case TOKEN_AND:   return "and";
    case TOKEN_OR:    return "or";
    case TOKEN_EQ:    return "=";
    default:          return ""; //unreachable
    }
}

static void print_literal(struct LiteralNode *node, u32 offset) {
    printf("Literal %.*s", node->base.span.length, node->base.span.start);
}

static void print_ident(struct IdentNode *node, u32 offset) {
    printf("Ident %.*s", node->base.span.length, node->base.span.start);
}

static void print_unary(struct UnaryNode *node, u32 offset) {
    printf("Unary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.length, node->base.span.start);
    print_node(node->rhs, offset + 2);
}

static void print_binary(struct BinaryNode *node, u32 offset) {
    printf("Binary\n");
    printf("%*s", offset + 2, "");
    printf("%s", bin_op_to_str(node->op_tag));
    print_node(node->lhs, offset + 2);
    print_node(node->rhs, offset + 2);
}

static void print_fn_call(struct FnCallNode *node, u32 offset) {
    printf("FnCall");    
    print_node(node->lhs, offset + 2);
    for (i32 i = 0; i < node->arity; i++)
        print_node(node->args[i], offset + 2);
}

static void print_block(struct BlockNode *node, u32 offset) {
    printf("Block");
    for (i32 i = 0; i < node->count; i++)
        print_node(node->stmts[i], offset + 2);
}

static void print_if(struct IfNode *node, u32 offset) {
    printf("If");
    print_node(node->cond, offset + 2);
    print_node((struct Node*)node->thn, offset + 2);
    if (node->els)
        print_node((struct Node*)node->els, offset + 2);
}

static void print_expr_stmt(struct ExprStmtNode *node, u32 offset) {
    printf("ExprStmt");
    print_node(node->expr, offset + 2);
}

static void print_return(struct ReturnNode *node, u32 offset) {
    printf("Return");
    if (node->expr)
        print_node(node->expr, offset + 2);
}

static void print_var_decl(struct VarDeclNode *node, u32 offset) {
    printf("VarDecl\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.length, node->base.span.start);
    if (node->init)
        print_node(node->init, offset + 2);
}

static void print_fn_decl(struct FnDeclNode *node, u32 offset) {
    printf("FnDeclNode\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.length, node->base.span.start);
    print_node(node->body, offset + 2);
}

// TEMP remove when we add functions
static void print_print(struct PrintNode *node, u32 offset) {
    printf("Print");
    print_node(node->expr, offset + 2);
}

void print_node(struct Node *node, u32 offset) {
    if (offset != 0)
        printf("\n");
    printf("%*s(", offset, "");
    switch (node->tag) {
    case NODE_LITERAL:   print_literal((struct LiteralNode*)node, offset); break;
    case NODE_IDENT:     print_ident((struct IdentNode*)node, offset); break;
    case NODE_UNARY:     print_unary((struct UnaryNode*)node, offset); break;
    case NODE_BINARY:    print_binary((struct BinaryNode*)node, offset); break;
    case NODE_FN_CALL:   print_fn_call((struct FnCallNode*)node, offset); break;
    case NODE_BLOCK:     print_block((struct BlockNode*)node, offset); break;
    case NODE_IF:        print_if((struct IfNode*)node, offset); break;
    case NODE_EXPR_STMT: print_expr_stmt((struct ExprStmtNode*)node, offset); break;
    case NODE_VAR_DECL:  print_var_decl((struct VarDeclNode*)node, offset); break;
    case NODE_FN_DECL:   print_fn_decl((struct FnDeclNode*)node, offset); break;
    case NODE_RETURN:    print_return((struct ReturnNode*)node, offset); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     print_print((struct PrintNode*)node, offset); break; 
    }
    printf(")");
    if (offset == 0)
        printf("\n");
}