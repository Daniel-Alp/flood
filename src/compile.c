#include <stdlib.h>
#include "compile.h"

#define BIN

// currently we treat everything as doubles (including booleans)
// later I'll change to tagged unions, and later later I'll do NaN tagging

void init_compiler(struct Compiler *compiler) {
    init_chunk(&compiler->chunk);
    compiler->stack_count = 0;
}

void release_compiler(struct Compiler *compiler) {
    release_chunk(&compiler->chunk);
    compiler->stack_count = 0;
}

// Note that VM fetches instr, increments ip, executes instr, in that order
// jump instructions follow this format
//      OP_JUMP
//      hi
//      lo
// The VM fetches OP_JUMP, ip is pointing past OP_JUMP (at hi)
// The VM reads a short, ip is pointing past lo
// The ip is incremented, and points at the instr after the skipped block

static u32 emit_jump(struct Compiler *compiler, enum OpCode op) {
    u32 offset = compiler->chunk.count;
    emit_byte(&compiler->chunk, op);
    compiler->chunk.count += 2;
    return offset;
}

static void patch_jump(struct Compiler *compiler, u32 offset) {
    // as seen above, offset points to the OP_JUMP instr
    // compiler->chunk.count points to the to-be-executed instr
    u32 jump = compiler->chunk.count - (offset+3);
    // TODO error if jump > max u16
    compiler->chunk.code[offset+1] = (jump >> 8) & 0xff;
    compiler->chunk.code[offset+2] = jump & 0xff; 
}

static void compile_node(struct Compiler *compiler, struct Node *node, struct SymTable *st);

static void compile_literal(struct Compiler *compiler, struct LiteralNode *node) {
    switch(node->lit_tag) {
    case TOKEN_TRUE:
        emit_byte(&compiler->chunk, OP_TRUE);
        break;
    case TOKEN_FALSE:
        emit_byte(&compiler->chunk, OP_FALSE);
        break;
    case TOKEN_NUMBER:
        emit_byte(&compiler->chunk, OP_GET_CONST);
        Value val = strtod(node->base.span.start, NULL);
        emit_byte(&compiler->chunk, add_constant(&compiler->chunk, val));
        break;
    }
}

static void compile_ident(struct Compiler *compiler, struct IdentNode *node, struct SymTable *st) {
    i32 idx = st->symbols[node->id].idx;
    emit_byte(&compiler->chunk, OP_GET_LOCAL);
    emit_byte(&compiler->chunk, idx);
}

static void compile_unary(struct Compiler *compiler, struct UnaryNode *node, struct SymTable *st) {
    compile_node(compiler, node->rhs, st);
    if (node->op_tag == TOKEN_MINUS)
        emit_byte(&compiler->chunk, OP_NEGATE);
    else if (node->op_tag == TOKEN_NOT)
        emit_byte(&compiler->chunk, OP_NOT);
}

static void compile_binary(struct Compiler *compiler, struct BinaryNode *node, struct SymTable *st) {
    enum TokenTag op_tag = node->op_tag;
    if (op_tag == TOKEN_EQ) {
        compile_node(compiler, node->rhs, st);
        if (node->lhs->tag == NODE_IDENT) {
            i32 idx = st->symbols[((struct IdentNode*)node->lhs)->id].idx;
            emit_byte(&compiler->chunk, OP_SET_LOCAL);
            emit_byte(&compiler->chunk, idx);
        }
    } else if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
        compile_node(compiler, node->lhs, st);
        u32 offset = emit_jump(compiler, op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE);
        emit_byte(&compiler->chunk, OP_POP);
        compile_node(compiler, node->rhs, st);
        patch_jump(compiler, offset);
    } else {
        compile_node(compiler, node->lhs, st);
        compile_node(compiler, node->rhs, st);
        switch (op_tag) {
        case TOKEN_PLUS:  emit_byte(&compiler->chunk, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(&compiler->chunk, OP_SUB); break;
        case TOKEN_STAR:  emit_byte(&compiler->chunk, OP_MUL); break;
        case TOKEN_SLASH: emit_byte(&compiler->chunk, OP_DIV); break;
        case TOKEN_LT:    emit_byte(&compiler->chunk, OP_LT); break; 
        case TOKEN_LEQ:   emit_byte(&compiler->chunk, OP_LEQ); break;
        case TOKEN_GT:    emit_byte(&compiler->chunk, OP_GT); break;
        case TOKEN_GEQ:   emit_byte(&compiler->chunk, OP_GEQ); break;
        case TOKEN_EQ_EQ: emit_byte(&compiler->chunk, OP_EQ_EQ); break;
        case TOKEN_NEQ:   emit_byte(&compiler->chunk, OP_NEQ); break;
        }
    }
}

static void compile_block(struct Compiler *compiler, struct BlockNode *node, struct SymTable *st) {
    u32 stack_count = compiler->stack_count;
    for (i32 i = 0; i < node->count; i++)
        compile_node(compiler, node->stmts[i], st);
    // TODO test POP_N command
    for (;compiler->stack_count > stack_count; compiler->stack_count--)
        emit_byte(&compiler->chunk, OP_POP);
}

static void compile_if(struct Compiler *compiler, struct IfNode *node, struct SymTable *st) {
    compile_node(compiler, node->cond, st);
    // skip over thn block
    u32 offset1 = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(&compiler->chunk, OP_POP);
    compile_block(compiler, node->thn, st);
    if (node->els) {
        // skip over else block
        u32 offset2 = emit_jump(compiler, OP_JUMP);
        patch_jump(compiler, offset1);
        emit_byte(&compiler->chunk, OP_POP);
        compile_block(compiler, node->els, st);
        patch_jump(compiler, offset2);
    } else {
        patch_jump(compiler, offset1);
    }
}

static void compile_expr_stmt(struct Compiler *compiler, struct ExprStmtNode *node, struct SymTable *st) {
    compile_node(compiler, node->expr, st);
    emit_byte(&compiler->chunk, OP_POP);
}

// TEMP remove when we add functions
static void compile_print_node(struct Compiler *compiler, struct PrintNode *node, struct SymTable *st) {
    compile_node(compiler, node->expr, st);
    emit_byte(&compiler->chunk, OP_PRINT);
}

static void compile_var_decl_node(struct Compiler *compiler, struct VarDeclNode *node, struct SymTable *st) {
    st->symbols[node->id].idx = compiler->stack_count;
    compiler->stack_count++;
    if (node->init)
        compile_node(compiler, node->init, st);
    else 
        emit_byte(&compiler->chunk, OP_DUMMY);
}

static void compile_node(struct Compiler *compiler, struct Node *node, struct SymTable *st) {
    switch (node->tag) {
    case NODE_LITERAL:   compile_literal(compiler, (struct LiteralNode*)node); break;
    case NODE_IDENT:     compile_ident(compiler, (struct IdentNode*)node, st); break;
    case NODE_UNARY:     compile_unary(compiler, (struct UnaryNode*)node, st); break;
    case NODE_BINARY:    compile_binary(compiler, (struct BinaryNode*)node, st); break;
    case NODE_BLOCK:     compile_block(compiler, (struct BlockNode*)node, st); break;
    case NODE_IF:        compile_if(compiler, (struct IfNode*)node, st); break;
    case NODE_EXPR_STMT: compile_expr_stmt(compiler, (struct ExprStmtNode*)node, st); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     compile_print_node(compiler, (struct PrintNode*)node, st); break;
    case NODE_VAR_DECL:  compile_var_decl_node(compiler, (struct VarDeclNode*)node, st); break;
    }
}

void compile(struct Compiler *compiler, struct Node *node, struct SymTable *st) {
    compile_node(compiler, node, st);
    emit_byte(&compiler->chunk, OP_RETURN);
}