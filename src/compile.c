#include <stdlib.h>
#include "memory.h"
#include "compile.h"
#include "symbol.h"
#include "parse.h"

void init_compiler(struct Compiler *compiler, struct SymArr *arr) 
{
    compiler->stack_pos = 0;
    compiler->global_cnt = 0;
    compiler->sym_arr = arr;
}

void release_compiler(struct Compiler *compiler)
{
    compiler->stack_pos = 0;
    compiler->global_cnt = 0;
    // compiler does not own sym_arr
    compiler->sym_arr = NULL;
}

static struct Chunk *cur_chunk(struct Compiler *compiler) 
{
    return &compiler->fn->chunk;
}

static struct Symbol *symbols(struct Compiler *compiler)
{
    return compiler->sym_arr->symbols;
}

static u32 emit_jump(struct Compiler *compiler, enum OpCode op) {
    u32 offset = cur_chunk(compiler)->cnt;
    emit_byte(cur_chunk(compiler), op);
    // skip two bytes for jump
    //      OP_JUMP
    //      hi
    //      lo
    cur_chunk(compiler)->cnt += 2;
    return offset;
}

static void patch_jump(struct Compiler *compiler, u32 offset) {
    // offset points to the OP_JUMP instr
    // compiler->chunk.count points to the to-be-executed instr
    u32 jump =  cur_chunk(compiler)->cnt - (offset+3);
    // TODO error if jump > max u16
    cur_chunk(compiler)->code[offset+1] = (jump >> 8) & 0xff;
    cur_chunk(compiler)->code[offset+2] = jump & 0xff; 
}

static void compile_node(struct Compiler *compiler, struct Node *node);

static void compile_literal(struct Compiler *compiler, struct LiteralNode *node) 
{
    switch(node->lit_tag) {
    case TOKEN_TRUE:
        emit_byte(cur_chunk(compiler), OP_TRUE);
        break;
    case TOKEN_FALSE:
        emit_byte(cur_chunk(compiler), OP_FALSE);
        break;
    case TOKEN_NUMBER:
        emit_byte(cur_chunk(compiler), OP_GET_CONST);
        Value val = NUM_VAL(strtod(node->base.span.start, NULL));
        emit_byte(cur_chunk(compiler), add_constant(cur_chunk(compiler), val));
        break;
    }
}

static void compile_ident(struct Compiler *compiler, struct IdentNode *node) 
{
    struct Symbol sym = symbols(compiler)[node->id];
    if (sym.flags & FLAG_LOCAL)
        emit_byte(cur_chunk(compiler), OP_GET_LOCAL);
    else // TODO OP_GET_GLOBAL_LONG
        emit_byte(cur_chunk(compiler), OP_GET_GLOBAL);
    emit_byte(cur_chunk(compiler), sym.idx);
}

static void compile_unary(struct Compiler *compiler, struct UnaryNode *node) 
{
    compile_node(compiler, node->rhs);
    if (node->op_tag == TOKEN_MINUS)
        emit_byte(cur_chunk(compiler), OP_NEGATE);
    else if (node->op_tag == TOKEN_NOT)
        emit_byte(cur_chunk(compiler), OP_NOT);
}

static void compile_binary(struct Compiler *compiler, struct BinaryNode *node) 
{
    enum TokenTag op_tag = node->op_tag;
    if (op_tag == TOKEN_EQ) {
        compile_node(compiler, node->rhs);
        if (node->lhs->tag == NODE_IDENT) {
            emit_byte(cur_chunk(compiler), OP_SET_LOCAL);
            emit_byte(cur_chunk(compiler), symbols(compiler)[((struct IdentNode*)node->lhs)->id].idx);
        }
    } else if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
        compile_node(compiler, node->lhs);
        u32 offset = emit_jump(compiler, op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE);
        emit_byte(cur_chunk(compiler), OP_POP);
        compile_node(compiler, node->rhs);
        patch_jump(compiler, offset);
    } else {
        compile_node(compiler, node->lhs);
        compile_node(compiler, node->rhs);
        switch (op_tag) {
        case TOKEN_PLUS:  emit_byte(cur_chunk(compiler), OP_ADD); break;
        case TOKEN_MINUS: emit_byte(cur_chunk(compiler), OP_SUB); break;
        case TOKEN_STAR:  emit_byte(cur_chunk(compiler), OP_MUL); break;
        case TOKEN_SLASH: emit_byte(cur_chunk(compiler), OP_DIV); break;
        case TOKEN_LT:    emit_byte(cur_chunk(compiler), OP_LT); break; 
        case TOKEN_LEQ:   emit_byte(cur_chunk(compiler), OP_LEQ); break;
        case TOKEN_GT:    emit_byte(cur_chunk(compiler), OP_GT); break;
        case TOKEN_GEQ:   emit_byte(cur_chunk(compiler), OP_GEQ); break;
        case TOKEN_EQEQ:  emit_byte(cur_chunk(compiler), OP_EQEQ); break;
        case TOKEN_NEQ:   emit_byte(cur_chunk(compiler), OP_NEQ); break;
        }
    }
}

static void compile_fn_call(struct Compiler *compiler, struct FnCallNode *node) 
{
    compile_node(compiler, node->lhs);
    for (i32 i = 0; i < node->arity; i++)
        compile_node(compiler, node->args[i]);
    emit_byte(cur_chunk(compiler), OP_CALL);
    emit_byte(cur_chunk(compiler), node->arity);
    if (node->arity == 1) {
        emit_byte(cur_chunk(compiler), OP_POP);
    } else if (node->arity > 1) {
        emit_byte(cur_chunk(compiler), OP_POP_N);
        emit_byte(cur_chunk(compiler), node->arity);
    }
    emit_byte(cur_chunk(compiler), OP_POP);
}

static void compile_var_decl(struct Compiler *compiler, struct VarDeclNode *node) 
{
    symbols(compiler)[node->id].idx = compiler->stack_pos;
    compiler->stack_pos++;
    if (node->init)
        compile_node(compiler, node->init);
    else 
        emit_byte(cur_chunk(compiler), OP_NIL);
}

static void compile_block(struct Compiler *compiler, struct BlockNode *node) 
{
    u32 stack_pos = compiler->stack_pos;
    for (i32 i = 0; i < node->cnt; i++)
        compile_node(compiler, node->stmts[i]);
    // TODO handle more than 256 locals
    u32 n = compiler->stack_pos - stack_pos;
    if (n == 1) {
        emit_byte(cur_chunk(compiler), OP_POP);
    } else if (n > 1) {
        emit_byte(cur_chunk(compiler), OP_POP_N);
        emit_byte(cur_chunk(compiler), n);
    }
    compiler->stack_pos = stack_pos;
}

static void compile_if(struct Compiler *compiler, struct IfNode *node) 
{
    compile_node(compiler, node->cond);
    // skip over thn block
    u32 offset1 = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(cur_chunk(compiler), OP_POP);
    compile_block(compiler, node->thn);
    if (node->els) {
        // skip over else block
        u32 offset2 = emit_jump(compiler, OP_JUMP);
        patch_jump(compiler, offset1);
        emit_byte(cur_chunk(compiler), OP_POP);
        compile_block(compiler, node->els);
        patch_jump(compiler, offset2);
    } else {
        patch_jump(compiler, offset1);
        emit_byte(cur_chunk(compiler), OP_POP);
    }
}

static void compile_expr_stmt(struct Compiler *compiler, struct ExprStmtNode *node) 
{
    compile_node(compiler, node->expr);
    emit_byte(cur_chunk(compiler), OP_POP);
}

static void compile_return(struct Compiler *compiler, struct ReturnNode *node) 
{
    if (node->expr)
        compile_node(compiler, node->expr);
    else
        emit_byte(cur_chunk(compiler), OP_NIL);
    emit_byte(cur_chunk(compiler), OP_RETURN);
}

static void compile_print(struct Compiler *compiler, struct PrintNode *node) 
{
    compile_node(compiler, node->expr);
    emit_byte(cur_chunk(compiler), OP_PRINT);
}


static void compile_node(struct Compiler *compiler, struct Node *node) 
{
    switch (node->tag) {
    case NODE_LITERAL:   compile_literal(compiler, (struct LiteralNode*)node); break;
    case NODE_IDENT:     compile_ident(compiler, (struct IdentNode*)node); break;
    case NODE_UNARY:     compile_unary(compiler, (struct UnaryNode*)node); break;
    case NODE_BINARY:    compile_binary(compiler, (struct BinaryNode*)node); break;
    case NODE_FN_CALL:   compile_fn_call(compiler, (struct FnCallNode*)node); break;
    case NODE_BLOCK:     compile_block(compiler, (struct BlockNode*)node); break;
    case NODE_IF:        compile_if(compiler, (struct IfNode*)node); break;
    case NODE_EXPR_STMT: compile_expr_stmt(compiler, (struct ExprStmtNode*)node); break;
    case NODE_VAR_DECL:  compile_var_decl(compiler, (struct VarDeclNode*)node); break;
    case NODE_RETURN:    compile_return(compiler, (struct ReturnNode*)node); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     compile_print(compiler, (struct PrintNode*)node); break; 
    }
}

struct FnObj *compile_module(struct Compiler *compiler, struct ModuleNode *node) 
{
    struct FnObj *script = allocate(sizeof(struct FnObj));
    // TODO fix
    struct Span span = {.start = "script", .length = 6};
    init_fn_obj(script, span, 0);

    compiler->fn = script;
    for (i32 i = 0; i < node->cnt; i++) {
        // TODO global variables
        // Compile function body, wrap in OBJ_FN, add to constant table of script
        struct FnDeclNode* fn_decl = (struct FnDeclNode*)node->stmts[i];
        
        struct FnObj *fn = allocate(sizeof(struct FnObj));
        init_fn_obj(fn, fn_decl->base.span, fn_decl->arity);

        // TODO clean up
        u32 stack_pos = compiler->stack_pos;
        for (i32 i = 0; i < fn_decl->arity; i++) {
            symbols(compiler)[fn_decl->params[i].id].idx = compiler->stack_pos;
            compiler->stack_pos++;
        }
        compiler->fn = fn;
        compile_node(compiler, (struct Node*)fn_decl->body);
        emit_byte(cur_chunk(compiler), OP_NIL);
        emit_byte(cur_chunk(compiler), OP_RETURN);
        compiler->stack_pos = stack_pos;

        compiler->fn = script;
        emit_byte(cur_chunk(compiler), OP_GET_CONST);
        emit_byte(cur_chunk(compiler), add_constant(&script->chunk, OBJ_VAL((struct Obj*)fn)));
        emit_byte(cur_chunk(compiler), OP_SET_GLOBAL);
        // TOOD handle > 256 globals
        emit_byte(cur_chunk(compiler), compiler->global_cnt);
        emit_byte(cur_chunk(compiler), OP_POP);
        compiler->global_cnt++;
    }
    return script;
}