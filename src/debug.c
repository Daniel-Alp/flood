#include <stdio.h>
#include "debug.h"
#include "object.h"

// we cannot use the token span for printing the binary operator because  
//      x += 3;
// is desugared into
//      x = x + 3;
const char *binop_str[] = {
    [TOKEN_PLUS]        = "+",
    [TOKEN_MINUS]       = "-",
    [TOKEN_STAR]        = "*",
    [TOKEN_SLASH]       = "/",
    [TOKEN_SLASH_SLASH] = "//", 
    [TOKEN_PERCENT]     = "%",
    [TOKEN_LT]          = "<",
    [TOKEN_LEQ]         = "<=",
    [TOKEN_GT]          = ">",
    [TOKEN_GEQ]         = ">=",
    [TOKEN_EQEQ]        = "==",
    [TOKEN_NEQ]         = "!=",
    [TOKEN_AND]         = "and",
    [TOKEN_OR]          = "or",
    [TOKEN_EQ]          = "=",
    [TOKEN_L_SQUARE]    = "[]"
};

static void print_atom(struct AtomNode *node, i32 offset) 
{
    printf("Atom %.*s", node->base.span.len, node->base.span.start);
}

static void print_list(struct ListNode *node, i32 offset)
{
    printf("List");
    for (i32 i = 0; i < node->cnt; i++)
        print_node(node->items[i], offset + 2);
}

static void print_ident(struct IdentNode *node, i32 offset) 
{
    printf("Ident %.*s id: %d", node->base.span.len, node->base.span.start, node->id);
}

static void print_unary(struct UnaryNode *node, i32 offset) 
{
    printf("Unary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->base.span.len, node->base.span.start);
    print_node(node->rhs, offset + 2);
}

static void print_binary(struct BinaryNode *node, i32 offset) 
{
    printf("Binary\n");
    printf("%*s", offset + 2, "");
    printf("%s", binop_str[node->op_tag]);
    print_node(node->lhs, offset + 2);
    print_node(node->rhs, offset + 2);
}

static void print_get_prop(struct PropNode *node, i32 offset)
{
    printf("GetProp");
    print_node(node->lhs, offset + 2);
    printf("\n%*s", offset + 2, "");
    printf("%.*s", node->prop.len, node->prop.start);
}

static void print_fn_call(struct FnCallNode *node, i32 offset) 
{
    printf("FnCall");    
    print_node(node->lhs, offset + 2);
    for (i32 i = 0; i < node->arity; i++)
        print_node(node->args[i], offset + 2);
}

static void print_block(struct BlockNode *node, i32 offset) 
{
    printf("Block");
    for (i32 i = 0; i < node->cnt; i++)
        print_node(node->stmts[i], offset + 2);
}

static void print_if(struct IfNode *node, i32 offset) 
{
    printf("If");
    print_node(node->cond, offset + 2);
    print_node((struct Node*)node->thn, offset + 2);
    if (node->els)
        print_node((struct Node*)node->els, offset + 2);
}

static void print_expr_stmt(struct ExprStmtNode *node, i32 offset) 
{
    printf("ExprStmt");
    print_node(node->expr, offset + 2);
}

static void print_return(struct ReturnNode *node, i32 offset) 
{
    printf("Return");
    if (node->expr)
        print_node(node->expr, offset + 2);
}

static void print_var_decl(struct VarDeclNode *node, i32 offset) 
{
    printf("VarDecl\n");
    printf("%*s", offset + 2, "");
    printf("%.*s id: %d", node->base.span.len, node->base.span.start, node->id);
    if (node->init)
        print_node(node->init, offset + 2);
}

static void print_fn_decl(struct FnDeclNode *node, i32 offset) 
{
    printf("FnDeclNode\n");
    printf("%*s", offset + 2, "");
    printf("%.*s(", node->base.span.len, node->base.span.start);
    for (i32 i = 0; i < node->arity-1; i++) {
        struct IdentNode param = node->params[i];
        printf("%.*s id: %d, ", param.base.span.len, param.base.span.start, param.id);
    }
    if (node->arity > 0) {
        struct IdentNode param = node->params[node->arity-1];
        printf("%.*s id: %d", param.base.span.len, param.base.span.start, param.id);
    }
    printf(") id: %d\n", node->id);
    
    printf("%*s", offset + 2, "");
    printf("stack_captures:  [");
    for (i32 i = 0; i < node->stack_capture_cnt-1; i++)
        printf("%d, ", node->stack_captures[i]);
    if (node->stack_capture_cnt > 0)
        printf("%d", node->stack_captures[node->stack_capture_cnt-1]);
    printf("]\n");
    printf("%*s", offset + 2, "");
    printf("parent_captures: [");
    for (i32 i = 0; i < node->parent_capture_cnt-1; i++)
        printf("%d, ", node->parent_captures[i]);
    if (node->parent_capture_cnt > 0)
        printf("%d", node->parent_captures[node->parent_capture_cnt-1]);
    printf("]");
    print_node((struct Node*)node->body, offset + 2);
}

static void print_import(struct ImportNode *node, i32 offset)
{
    printf("ImportNode %.*s %.*s", node->path.len, node->path.start, node->base.span.len, node->base.span.start);
}

// TEMP remove when we add functions
static void print_print(struct PrintNode *node, i32 offset) 
{
    printf("Print");
    print_node(node->expr, offset + 2);
}

void print_node(struct Node *node, i32 offset) 
{
    if (offset != 0)
        printf("\n");
    printf("%*s(", offset, "");
    switch (node->tag) {
    case NODE_ATOM:      print_atom((struct AtomNode*)node, offset); break;
    case NODE_LIST:      print_list((struct ListNode*)node, offset); break;
    case NODE_IDENT:     print_ident((struct IdentNode*)node, offset); break;
    case NODE_UNARY:     print_unary((struct UnaryNode*)node, offset); break;
    case NODE_BINARY:    print_binary((struct BinaryNode*)node, offset); break;
    case NODE_PROP:  print_get_prop((struct PropNode*)node, offset); break;
    case NODE_FN_CALL:   print_fn_call((struct FnCallNode*)node, offset); break;
    case NODE_BLOCK:     print_block((struct BlockNode*)node, offset); break;
    case NODE_IF:        print_if((struct IfNode*)node, offset); break;
    case NODE_EXPR_STMT: print_expr_stmt((struct ExprStmtNode*)node, offset); break;
    case NODE_VAR_DECL:  print_var_decl((struct VarDeclNode*)node, offset); break;
    case NODE_FN_DECL:   print_fn_decl((struct FnDeclNode*)node, offset); break;
    case NODE_IMPORT:    print_import((struct ImportNode*)node, offset); break;
    case NODE_RETURN:    print_return((struct ReturnNode*)node, offset); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     print_print((struct PrintNode*)node, offset); break; 
    }
    printf(")");
    if (offset == 0)
        printf("\n");
}

const char *opcode_str[] = {
    [OP_NIL]           = "OP_NIL",
    [OP_TRUE]          = "OP_TRUE",
    [OP_FALSE]         = "OP_FALSE",
    [OP_ADD]           = "OP_ADD",
    [OP_SUB]           = "OP_SUB", 
    [OP_MUL]           = "OP_MUL",
    [OP_DIV]           = "OP_DIV",
    [OP_FLOORDIV]      = "OP_FLOORDIV",
    [OP_MOD]           = "OP_MOD",
    [OP_LT]            = "OP_LT",
    [OP_LEQ]           = "OP_LEQ",
    [OP_GT]            = "OP_GT",
    [OP_GEQ]           = "OP_GEQ",
    [OP_EQEQ]          = "OP_EQEQ",
    [OP_NEQ]           = "OP_NEQ",
    [OP_NEGATE]        = "OP_NEGATE",
    [OP_NOT]           = "OP_NOT",
    [OP_LIST]          = "OP_LIST",
    [OP_CLOSURE]       = "OP_CLOSURE",
    [OP_GET_CONST]     = "OP_GET_CONST",
    [OP_GET_LOCAL]     = "OP_GET_LOCAL",
    [OP_SET_LOCAL]     = "OP_SET_LOCAL",
    [OP_HEAPVAL]       = "OP_HEAPVAL",
    [OP_GET_HEAPVAL]   = "OP_GET_HEAPVAL",
    [OP_SET_HEAPVAL]   = "OP_SET_HEAPVAL",
    [OP_GET_CAPTURED]  = "OP_GET_CAPTURED",
    [OP_SET_CAPTURED]  = "OP_SET_CAPTURED",
    // TEMP remove globals when we added user-defined classes
    [OP_GET_GLOBAL]    = "OP_GET_GLOBAL",
    [OP_SET_GLOBAL]    = "OP_SET_GLOBAL",
    [OP_GET_SUBSCR]    = "OP_GET_SUBSCR",
    [OP_SET_SUBSCR]    = "OP_SET_SUBSCR",
    [OP_GET_PROP]      = "OP_GET_PROP",
    [OP_JUMP_IF_FALSE] = "OP_JUMP_IF_FALSE",
    [OP_JUMP_IF_TRUE]  = "OP_JUMP_IF_TRUE",
    [OP_JUMP]          = "OP_JUMP",
    [OP_CALL]          = "OP_CALL",
    [OP_RETURN]        = "OP_RETURN",
    [OP_POP]           = "OP_POP",
    [OP_POP_N]         = "OP_POP_N",
    [OP_PRINT]         = "OP_PRINT"
};

// TODO currently this instruction is a bit confusing because for constants we print their value
// but for GET/SET we print the index. Would make a bit more sense to print the name of the ident
void disassemble_chunk(struct Chunk *chunk, const char *name)
{
    printf("     [disassembly for %s]\n", name);
    for (i32 i = 0; i < chunk->cnt; i++) {
        printf("%4d | ", i);
        u8 op = chunk->code[i];
        printf("%-20s", opcode_str[op]);
        switch (op) {
        case OP_GET_LOCAL:     
        case OP_SET_LOCAL:   
        case OP_GET_HEAPVAL:
        case OP_SET_HEAPVAL:
        case OP_GET_CAPTURED:
        case OP_SET_CAPTURED: 
        case OP_GET_GLOBAL:   
        case OP_SET_GLOBAL:    
        case OP_CALL:          
        case OP_POP_N:
        case OP_LIST: 
        case OP_HEAPVAL:
            printf("%d\n", chunk->code[++i]); 
            break;
        case OP_JUMP_IF_FALSE: 
        case OP_JUMP_IF_TRUE:
        case OP_JUMP:          
            printf("%d\n", (chunk->code[++i] << 8) + chunk->code[++i]);
            break;
        case OP_GET_PROP:
        case OP_GET_CONST: {
            print_val(chunk->constants.vals[chunk->code[++i]]);
            printf("\n");
            break;
        }
        case OP_CLOSURE: {
            i32 stack_captures = chunk->code[++i];
            printf("%d\n", stack_captures); 
            i32 parent_captures = chunk->code[++i];
            printf("     | %*s%d\n", 20, "", parent_captures);
            for (i32 j = 0; j < stack_captures + parent_captures; j++)
                printf("     | %*s%d\n", 20, "", chunk->code[++i]);
            break;
        }      
        default:
            printf("\n");
        }
    }    
    printf("\n");
}

void print_stack(struct VM *vm, Value *sp, Value *bp) 
{
    printf("    [value stack]\n");
    i32 i = 0;
    while (vm->val_stack + i != sp) {
        if (vm->val_stack + i == bp)
            printf("bp > ");
        else    
            printf("     ");
        print_val(vm->val_stack[i]);
        printf("\n");
        i++;
    }
    printf("sp >\n\n");
}