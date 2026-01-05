#include <stdio.h>
#include "debug.h"
#include "chunk.h"
#include "parse.h"
#include "scan.h"

// we cannot use the token span for printing the binary operator because  
//      x += 3;
// is desugared into
//      x = x + 3;
const char *binop_str(const TokenTag tag)
{
    switch(tag) {
    case TOKEN_PLUS:        return "+";
    case TOKEN_MINUS:       return "-";
    case TOKEN_STAR:        return "*";
    case TOKEN_SLASH:       return "/";
    case TOKEN_SLASH_SLASH: return "//"; 
    case TOKEN_PERCENT:     return "%";
    case TOKEN_LT:          return "<";
    case TOKEN_LEQ:         return "<=";
    case TOKEN_GT:          return ">";
    case TOKEN_GEQ:         return ">=";
    case TOKEN_EQEQ:        return "==";
    case TOKEN_NEQ:         return "!=";
    case TOKEN_AND:         return "and";
    case TOKEN_OR:          return "or";
    case TOKEN_EQ:          return "=";
    case TOKEN_L_SQUARE:    return "[]";
    default:                return nullptr;
    }
}

// TODO change pointers to references

static void print_atom(const AtomNode *node) 
{
    printf("Atom %.*s", node->span.len, node->span.start);
}

static void print_list(const ListNode *node, const i32 offset)
{
    printf("List");
    for (i32 i = 0; i < node->cnt; i++)
        print_node(node->items[i], offset + 2);
}

static void print_ident(const IdentNode *node) 
{
    printf("Ident %.*s id: %d", node->span.len, node->span.start, node->id);
}

static void print_unary(const UnaryNode *node, const i32 offset) 
{
    printf("Unary\n");
    printf("%*s", offset + 2, "");
    printf("%.*s", node->span.len, node->span.start);
    print_node(node->rhs, offset + 2);
}

static void print_binary(const BinaryNode *node, const i32 offset) 
{
    printf("Binary\n");
    printf("%*s", offset + 2, "");
    printf("%s", binop_str(node->op_tag));
    print_node(node->lhs, offset + 2);
    print_node(node->rhs, offset + 2);
}

static void print_dot(const PropertyNode *node, const i32 offset)
{
    printf("Property");
    print_node(node->lhs, offset + 2);
    printf("\n%*s", offset + 2, "");
    printf("%.*s", node->sym.len, node->sym.start);
}

static void print_call(const CallNode *node, const i32 offset) 
{
    printf("Call");    
    print_node(node->lhs, offset + 2);
    for (i32 i = 0; i < node->arity; i++)
        print_node(node->args[i], offset + 2);
}

static void print_block(const BlockNode *node, const i32 offset) 
{
    printf("Block");
    for (i32 i = 0; i < node->cnt; i++)
        print_node(node->stmts[i], offset + 2);
}

static void print_if(const IfNode *node, const i32 offset) 
{
    printf("If");
    print_node(node->cond, offset + 2);
    print_node(node->thn, offset + 2);
    if (node->els)
        print_node(node->els, offset + 2);
}

static void print_expr_stmt(const ExprStmtNode *node, const i32 offset) 
{
    printf("ExprStmt");
    print_node(node->expr, offset + 2);
}

static void print_return(const ReturnNode *node, const i32 offset) 
{
    printf("Return");
    if (node->expr)
        print_node(node->expr, offset + 2);
}

static void print_var_decl(const VarDeclNode *node, const i32 offset) 
{
    printf("VarDecl\n");
    printf("%*s", offset + 2, "");
    printf("%.*s id: %d", node->span.len, node->span.start, node->id);
    if (node->init)
        print_node(node->init, offset + 2);
}

static void print_fn_decl(const FnDeclNode *node, const i32 offset) 
{
    printf("FnDeclNode\n");
    printf("%*s", offset + 2, "");
    printf("%.*s(", node->span.len, node->span.start);
    for (i32 i = 0; i < node->arity-1; i++) {
        const IdentNode param = node->params[i];
        printf("%.*s id: %d, ", param.span.len, param.span.start, param.id);
    }
    if (node->arity > 0) {
        const IdentNode param = node->params[node->arity-1];
        printf("%.*s id: %d", param.span.len, param.span.start, param.id);
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
    print_node(node->body, offset + 2);
}

// TEMP remove when we add functions
static void print_print(const PrintNode *node, const i32 offset) 
{
    printf("Print");
    print_node(node->expr, offset + 2);
}

static void print_module(const ModuleNode *node, const i32 offset)
{
    printf("ModuleNode");
    for (i32 i = 0; i < node->cnt; i++)
        print_node(node->decls[i], offset + 2);
}

void print_node(const Node *node, const i32 offset) 
{
    if (offset != 0)
        printf("\n");
    printf("%*s(", offset, "");
    switch (node->tag) {
    case NODE_ATOM:        print_atom(static_cast<const AtomNode*>(node)); break;
    case NODE_LIST:        print_list(static_cast<const ListNode*>(node), offset); break;
    case NODE_IDENT:       print_ident(static_cast<const IdentNode*>(node)); break;
    case NODE_UNARY:       print_unary(static_cast<const UnaryNode*>(node), offset); break;
    case NODE_BINARY:      print_binary(static_cast<const BinaryNode*>(node), offset); break;
    case NODE_PROPERTY:    print_dot(static_cast<const PropertyNode*>(node), offset); break;
    case NODE_CALL:        print_call(static_cast<const CallNode*>(node), offset); break;
    case NODE_BLOCK:       print_block(static_cast<const BlockNode*>(node), offset); break;
    case NODE_IF:          print_if(static_cast<const IfNode*>(node), offset); break;
    case NODE_EXPR_STMT:   print_expr_stmt(static_cast<const ExprStmtNode*>(node), offset); break;
    case NODE_VAR_DECL:    print_var_decl(static_cast<const VarDeclNode*>(node), offset); break;
    case NODE_FN_DECL:     print_fn_decl(static_cast<const FnDeclNode*>(node), offset); break;
    case NODE_RETURN:      print_return(static_cast<const ReturnNode*>(node), offset); break;
    // TEMP remove when we add functions
    case NODE_PRINT:       print_print(static_cast<const PrintNode*>(node), offset); break; 
    case NODE_MODULE:      print_module(static_cast<const ModuleNode*>(node), offset); break;
    }
    printf(")");
    if (offset == 0)
        printf("\n");
}

const char* opcode_str(const OpCode op) {
    switch (op) {
    case OP_NULL:          return "OP_NULL";
    case OP_TRUE:          return "OP_TRUE";
    case OP_FALSE:         return "OP_FALSE";
    case OP_ADD:           return "OP_ADD";
    case OP_SUB:           return "OP_SUB";
    case OP_MUL:           return "OP_MUL";
    case OP_DIV:           return "OP_DIV";
    case OP_FLOORDIV:      return "OP_FLOORDIV";
    case OP_MOD:           return "OP_MOD";
    case OP_LT:            return "OP_LT";
    case OP_LEQ:           return "OP_LEQ";
    case OP_GT:            return "OP_GT";
    case OP_GEQ:           return "OP_GEQ";
    case OP_EQEQ:          return "OP_EQEQ";
    case OP_NEQ:           return "OP_NEQ";
    case OP_NEGATE:        return "OP_NEGATE";
    case OP_NOT:           return "OP_NOT";
    case OP_LIST:          return "OP_LIST";
    case OP_CLOSURE:       return "OP_CLOSURE";
    case OP_CLASS:         return "OP_CLASS";
    case OP_METHOD:        return "OP_METHOD";
    case OP_GET_CONST:     return "OP_GET_CONST";
    case OP_GET_LOCAL:     return "OP_GET_LOCAL";
    case OP_SET_LOCAL:     return "OP_SET_LOCAL";
    case OP_HEAPVAL:       return "OP_HEAPVAL";
    case OP_GET_HEAPVAL:   return "OP_GET_HEAPVAL";
    case OP_SET_HEAPVAL:   return "OP_SET_HEAPVAL";
    case OP_GET_CAPTURED:  return "OP_GET_CAPTURED";
    case OP_SET_CAPTURED:  return "OP_SET_CAPTURED";
    case OP_GET_GLOBAL:    return "OP_GET_GLOBAL";
    // TEMP remove globals when we added user-defined classes
    case OP_SET_GLOBAL:    return "OP_SET_GLOBAL";
    case OP_GET_SUBSCR:    return "OP_GET_SUBSCR";
    case OP_SET_SUBSCR:    return "OP_SET_SUBSCR";
    case OP_GET_FIELD:     return "OP_GET_FIELD";
    case OP_SET_FIELD:     return "OP_SET_FIELD";
    case OP_GET_METHOD:    return "OP_GET_METHOD";
    case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
    case OP_JUMP_IF_TRUE:  return "OP_JUMP_IF_TRUE";
    case OP_JUMP:          return "OP_JUMP";
    case OP_CALL:          return "OP_CALL";
    case OP_RETURN:        return "OP_RETURN";
    case OP_POP:           return "OP_POP";
    case OP_POP_N:         return "OP_POP_N";
    case OP_PRINT:         return "OP_PRINT";
    default:               return nullptr;
    }
}

// TODO currently this instruction is a bit confusing because for constants we print their value
// but for GET/SET we print the index. Would make a bit more sense to print the name of the ident
void disassemble_chunk(const Chunk &chunk, const char *name)
{
    printf("     [disassembly for %s]\n", name);
    for (i32 i = 0; i < chunk.code().len(); i++) {
        printf("%4d | ", i);
        const u8 op = chunk.code()[i];
        printf("%-20s", opcode_str(OpCode(op)));
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
            printf("%d\n", chunk.code()[++i]); 
            break;
        case OP_JUMP_IF_FALSE: 
        case OP_JUMP_IF_TRUE:
        case OP_JUMP: {
            const u16 offset = (i += 2, (chunk.code()[i-2] << 8) | chunk.code()[i-1]);
            printf("%d\n", offset);
            break;
        }       
        case OP_GET_FIELD:
        case OP_SET_FIELD:
        case OP_GET_METHOD:
        case OP_GET_CONST: {
            print_val(chunk.constants()[chunk.code()[++i]]);
            printf("\n");
            break;
        }
        case OP_CLOSURE: {
            const i32 stack_captures = chunk.code()[++i];
            printf("%d\n", stack_captures); 
            const i32 parent_captures = chunk.code()[++i];
            printf("     | %*s%d\n", 20, "", parent_captures);
            for (i32 j = 0; j < stack_captures + parent_captures; j++)
                printf("     | %*s%d\n", 20, "", chunk.code()[++i]);
            break;
        }      
        default:
            printf("\n");
        }
    }    
    printf("\n");
}

// void print_stack(const struct VM *vm, const Value *sp, const Value *bp) 
// {
//     printf("    [value stack]\n");
//     i32 i = 0;
//     while (vm->val_stack + i != sp) {
//         if (vm->val_stack + i == bp)
//             printf("bp > ");
//         else    
//             printf("     ");
//         print_val(vm->val_stack[i]);
//         printf("\n");
//         i++;
//     }
//     printf("sp >\n\n");
// }
