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

static void print_ty(struct TyNode *ty) {
    switch (ty->tag) {
    case TY_NUM:  printf("Num"); break;
    case TY_BOOL: printf("Bool"); break;
    case TY_VOID: printf("Void"); break;
    }
}

static void print_literal(struct LiteralNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Literal %.*s)", node->base.span.length, node->base.span.start);
}

static void print_ident(struct IdentNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Ident %.*s)", node->base.span.length, node->base.span.start);
}

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
    printf("%s\n", bin_op_to_str(node->op_tag));
    print_node(node->lhs, offset + 2);
    printf("\n");
    print_node(node->rhs, offset + 2);
    printf(")");
}

static void print_fn_call(struct FnCallNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(FnCall\n");    
    print_node(node->lhs, offset + 2);
    for (i32 i = 0; i < node->arity; i++) {
        printf("\n");
        print_node(node->args[i], offset + 2);
    }
    printf(")");
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

static void print_return(struct ReturnNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Return\n");
    print_node(node->expr, offset + 2);
    printf(")");
}

static void print_var_decl(struct VarDeclNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(VarDecl\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.length, node->base.span.start);
    if (node->ty_hint) {
        printf(": ");
        print_ty(node->ty_hint);
    }
    if (node->init) {
        printf("\n");
        print_node(node->init, offset + 2);
    }
    printf(")");
}

static void print_fn_decl(struct FnDeclNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(FnDeclNode\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.length, node->base.span.start);
    printf("(");
    for (i32 i = 0; i < node->arity; i++) {
        struct Span span = node->param_names[i]->base.span;
        printf("%.*s", span.length, span.start);
        printf(": ");
        print_ty(node->param_tys[i]);
        if (i < node->arity-1)
            printf(", ");
    }
    printf(") ");
    print_ty(node->ret_ty);
    printf("\n");
    print_block(node->body, offset + 2);
    printf(")");
}

// TEMP remove when we add functions
static void print_print(struct PrintNode *node, u32 offset) {
    printf("%*s", offset, "");
    printf("(Print\n");
    print_node(node->expr, offset + 2);
    printf(")");
}

void print_node(struct Node *node, u32 offset) {
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
    if (offset == 0)
        printf("\n");
}

void disassemble_chunk(struct Chunk *chunk) {
    for (i32 i = 0; i < chunk->count; i++) {
        printf("%4d | ", i);
        switch (chunk->code[i]) {
        case OP_DUMMY:         printf("OP_DUMMY\n"); break;
        case OP_TRUE:          printf("OP_TRUE\n"); break;
        case OP_FALSE:         printf("OP_FALSE\n"); break;
        case OP_ADD:           printf("OP_ADD\n"); break;
        case OP_SUB:           printf("OP_SUB\n"); break;
        case OP_MUL:           printf("OP_MUL\n"); break;        
        case OP_DIV:           printf("OP_DIV\n"); break;  
        case OP_LT:            printf("OP_LT\n"); break;  
        case OP_LEQ:           printf("OP_LEQ\n"); break;  
        case OP_GT:            printf("OP_GT\n"); break;  
        case OP_GEQ:           printf("OP_GEQ\n"); break;  
        case OP_EQ_EQ:         printf("OP_EQ_EQ\n"); break;
        case OP_NEQ:           printf("OP_NEQ\n"); break;
        case OP_NEGATE:        printf("OP_NEGATE\n"); break;
        case OP_NOT:           printf("OP_NOT\n"); break;    
        case OP_GET_CONST:
            printf("OP_GET_CONST     %.4f\n", chunk->constants.values[chunk->code[++i]]);
            break;
        case OP_GET_LOCAL:
            printf("OP_GET_LOCAL     %d\n", chunk->code[++i]);
            break;
        case OP_SET_LOCAL:
            printf("OP_SET_LOCAL     %d\n", chunk->code[++i]);           
            break;
        case OP_JUMP_IF_FALSE: 
            printf("OP_JUMP_IF_FALSE %d\n", (chunk->code[++i] << 8) + chunk->code[++i]); 
            break;
        case OP_JUMP_IF_TRUE:
            printf("OP_JUMP_IF_TRUE  %d\n", (chunk->code[++i] << 8) + chunk->code[++i]); 
            break;
        case OP_JUMP:
            printf("OP_JUMP          %d\n", (chunk->code[++i] << 8) + chunk->code[++i]); 
            break;       
        case OP_RETURN:        printf("OP_RETURN\n"); break;
        case OP_POP:           printf("OP_POP\n"); break;
        case OP_PRINT:         printf("OP_PRINT\n"); break;
        }
    }    
    printf("\n");
}