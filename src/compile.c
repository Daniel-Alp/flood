#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "compile.h"
#include "symbol.h"
#include "parse.h"

// TODO look at the line info being passed and check that it makes sense

void init_compiler(struct Compiler *compiler, struct SymArr *arr) 
{
    init_errlist(&compiler->errlist);
    compiler->stack_pos = 0;
    compiler->global_cnt = 0;
    compiler->main_fn_idx = -1;
    compiler->sym_arr = arr;
}

void release_compiler(struct Compiler *compiler)
{
    release_errlist(&compiler->errlist);
    compiler->stack_pos = 0;
    compiler->global_cnt = 0;
    compiler->main_fn_idx = -1;
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

static u32 emit_jump(struct Compiler *compiler, enum OpCode op, u32 line) 
{
    u32 offset = cur_chunk(compiler)->cnt;
    emit_byte(cur_chunk(compiler), op, line);
    // skip two bytes for jump
    //      OP_JUMP
    //      hi
    //      lo
    emit_byte(cur_chunk(compiler), 0, line);
    emit_byte(cur_chunk(compiler), 0, line);
    return offset;
}

static void patch_jump(struct Compiler *compiler, u32 offset) 
{
    // offset points to the OP_JUMP instr
    // compiler->chunk.count points to the to-be-executed instr
    u32 jump =  cur_chunk(compiler)->cnt - (offset+3);
    // TODO error if jump > max u16
    cur_chunk(compiler)->code[offset+1] = (jump >> 8) & 0xff;
    cur_chunk(compiler)->code[offset+2] = jump & 0xff; 
}

static void compile_node(struct Compiler *compiler, struct Node *node);

static void compile_atom(struct Compiler *compiler, struct AtomNode *node) 
{
    u32 line = node->base.span.line;
    switch(node->atom_tag) {
    case TOKEN_TRUE:
        emit_byte(cur_chunk(compiler), OP_TRUE, line);
        break;
    case TOKEN_FALSE:
        emit_byte(cur_chunk(compiler), OP_FALSE, line);
        break;
    case TOKEN_NUMBER:
        emit_byte(cur_chunk(compiler), OP_GET_CONST, line);
        Value val = MK_NUM(strtod(node->base.span.start, NULL));
        emit_byte(cur_chunk(compiler), add_constant(cur_chunk(compiler), val), line);
        break;
    case TOKEN_STRING:
        push_errlist(&compiler->errlist, node->base.span, "TODO");
        break;
    }
}

static void compile_ident(struct Compiler *compiler, struct IdentNode *node) 
{
    u32 line = node->base.span.line;
    struct Symbol sym = symbols(compiler)[node->id];
    if (sym.flags & FLAG_LOCAL)
        emit_byte(cur_chunk(compiler), OP_GET_LOCAL, line);
    else // TODO OP_GET_GLOBAL_LONG
        emit_byte(cur_chunk(compiler), OP_GET_GLOBAL, line);
    emit_byte(cur_chunk(compiler), sym.idx, line);
}

static void compile_list(struct Compiler *compiler, struct ListNode *node) 
{
    // TODO allow lists to be initialized with more than 256 elements
    // TODO handle lists being initialized with too many elements
    u32 line = node->base.span.line;
    for (i32 i = 0; i < node->cnt; i++)
        compile_node(compiler, node->items[i]);    
    emit_byte(cur_chunk(compiler), OP_LIST, line);
    emit_byte(cur_chunk(compiler), node->cnt, line);
}

static void compile_unary(struct Compiler *compiler, struct UnaryNode *node) 
{
    u32 line = node->base.span.line;
    compile_node(compiler, node->rhs);
    if (node->op_tag == TOKEN_MINUS)
        emit_byte(cur_chunk(compiler), OP_NEGATE, line);
    else if (node->op_tag == TOKEN_NOT)
        emit_byte(cur_chunk(compiler), OP_NOT, line);
}

static void compile_binary(struct Compiler *compiler, struct BinaryNode *node) 
{
    u32 line = node->base.span.line;
    enum TokenTag op_tag = node->op_tag;
    if (op_tag == TOKEN_EQ) {
        if (node->lhs->tag == NODE_IDENT) {
            compile_node(compiler, node->rhs);
            emit_byte(cur_chunk(compiler), OP_SET_LOCAL, line);
            emit_byte(cur_chunk(compiler), symbols(compiler)[((struct IdentNode*)node->lhs)->id].idx, line);
        } else if (node->lhs->tag == NODE_BINARY && ((struct BinaryNode*)node->lhs)->op_tag == TOKEN_L_SQUARE) {
            compile_node(compiler, ((struct BinaryNode*)node->lhs)->lhs);
            compile_node(compiler, ((struct BinaryNode*)node->lhs)->rhs);
            compile_node(compiler, node->rhs);
            emit_byte(cur_chunk(compiler), OP_SET_SUBSCR, line);
        }
    } else if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
        compile_node(compiler, node->lhs);
        u32 offset = emit_jump(compiler, op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE, line);
        emit_byte(cur_chunk(compiler), OP_POP, line);
        compile_node(compiler, node->rhs);
        patch_jump(compiler, offset);
    } else {
        compile_node(compiler, node->lhs);
        compile_node(compiler, node->rhs);
        switch (op_tag) {
        case TOKEN_PLUS:     emit_byte(cur_chunk(compiler), OP_ADD, line); break;
        case TOKEN_MINUS:    emit_byte(cur_chunk(compiler), OP_SUB, line); break;
        case TOKEN_STAR:     emit_byte(cur_chunk(compiler), OP_MUL, line); break;
        case TOKEN_SLASH:    emit_byte(cur_chunk(compiler), OP_DIV, line); break;
        case TOKEN_LT:       emit_byte(cur_chunk(compiler), OP_LT, line); break; 
        case TOKEN_LEQ:      emit_byte(cur_chunk(compiler), OP_LEQ, line); break;
        case TOKEN_GT:       emit_byte(cur_chunk(compiler), OP_GT, line); break;
        case TOKEN_GEQ:      emit_byte(cur_chunk(compiler), OP_GEQ, line); break;
        case TOKEN_EQEQ:     emit_byte(cur_chunk(compiler), OP_EQEQ, line); break;
        case TOKEN_NEQ:      emit_byte(cur_chunk(compiler), OP_NEQ, line); break;
        case TOKEN_L_SQUARE: emit_byte(cur_chunk(compiler), OP_GET_SUBSCR, line); break;
        }
    }
}

static void compile_get_prop(struct Compiler *compiler, struct GetPropNode *node)
{
    push_errlist(&compiler->errlist, node->base.span, "TODO");
}

static void compile_fn_call(struct Compiler *compiler, struct FnCallNode *node) 
{
    u32 line = node->base.span.line;
    compile_node(compiler, node->lhs);
    for (i32 i = 0; i < node->arity; i++)
        compile_node(compiler, node->args[i]);
    emit_byte(cur_chunk(compiler), OP_CALL, line);
    emit_byte(cur_chunk(compiler), node->arity, line);
    // pop args but don't pop func ptr (op return replaces func ptr with return val)
    if (node->arity == 1) {
        emit_byte(cur_chunk(compiler), OP_POP, line);
    } else if (node->arity > 1) {
        emit_byte(cur_chunk(compiler), OP_POP_N, line);
        emit_byte(cur_chunk(compiler), node->arity, line);
    }
}

static void compile_var_decl(struct Compiler *compiler, struct VarDeclNode *node) 
{
    symbols(compiler)[node->id].idx = compiler->stack_pos;
    compiler->stack_pos++;
    if (node->init)
        compile_node(compiler, node->init);
    else 
        emit_byte(cur_chunk(compiler), OP_NIL, node->base.span.line);
}

static void compile_block(struct Compiler *compiler, struct BlockNode *node) 
{
    u32 line = node->base.span.line;
    u32 stack_pos = compiler->stack_pos;
    for (i32 i = 0; i < node->cnt; i++)
        compile_node(compiler, node->stmts[i]);
    // TODO handle more than 256 locals
    u32 n = compiler->stack_pos - stack_pos;
    if (n == 1) {
        emit_byte(cur_chunk(compiler), OP_POP, line);
    } else if (n > 1) {
        emit_byte(cur_chunk(compiler), OP_POP_N, line);
        emit_byte(cur_chunk(compiler), n, line);
    }
    compiler->stack_pos = stack_pos;
}

static void compile_if(struct Compiler *compiler, struct IfNode *node) 
{
    u32 line = node->base.span.line;
    compile_node(compiler, node->cond);
    // skip over thn block
    u32 offset1 = emit_jump(compiler, OP_JUMP_IF_FALSE, line);
    emit_byte(cur_chunk(compiler), OP_POP, line);
    compile_block(compiler, node->thn);
    if (node->els) {
        // skip over else block
        u32 offset2 = emit_jump(compiler, OP_JUMP, line);
        patch_jump(compiler, offset1);
        emit_byte(cur_chunk(compiler), OP_POP, line);
        compile_block(compiler, node->els);
        patch_jump(compiler, offset2);
    } else {
        patch_jump(compiler, offset1);
        emit_byte(cur_chunk(compiler), OP_POP, line);
    }
}

static void compile_expr_stmt(struct Compiler *compiler, struct ExprStmtNode *node) 
{
    compile_node(compiler, node->expr);
    emit_byte(cur_chunk(compiler), OP_POP, node->base.span.line);
}

static void compile_return(struct Compiler *compiler, struct ReturnNode *node) 
{
    if (node->expr)
        compile_node(compiler, node->expr);
    else
        emit_byte(cur_chunk(compiler), OP_NIL, node->base.span.line);
    emit_byte(cur_chunk(compiler), OP_RETURN, node->base.span.line);
}

static void compile_print(struct Compiler *compiler, struct PrintNode *node) 
{
    compile_node(compiler, node->expr);
    emit_byte(cur_chunk(compiler), OP_PRINT, node->base.span.line);
}


static void compile_node(struct Compiler *compiler, struct Node *node) 
{
    switch (node->tag) {
    case NODE_ATOM:      compile_atom(compiler, (struct AtomNode*)node); break;
    case NODE_LIST:      compile_list(compiler, (struct ListNode*)node); break;
    case NODE_IDENT:     compile_ident(compiler, (struct IdentNode*)node); break;
    case NODE_UNARY:     compile_unary(compiler, (struct UnaryNode*)node); break;
    case NODE_BINARY:    compile_binary(compiler, (struct BinaryNode*)node); break;
    case NODE_GET_PROP:  compile_get_prop(compiler, (struct GetPropNode*)node); break;
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

struct FnObj *compile_file(struct VM *vm, struct Compiler *compiler, struct FileNode *node) 
{
    struct FnObj *script = (struct FnObj*)alloc_vm_obj(vm, sizeof(struct FnObj));
    char *name = allocate((6+1)*sizeof(char));
    strcpy(name, "script");
    init_fn_obj(script, name, 0);

    compiler->fn = script;
    for (i32 i = 0; i < node->cnt; i++) {
        // TODO global variables
        // Compile function body, wrap in OBJ_FN, add to constant table of script
        struct FnDeclNode* fn_decl = (struct FnDeclNode*)node->stmts[i];
        struct Span fn_span = fn_decl->base.span;

        if (fn_span.length == 4 && strncmp(fn_span.start, "main", 4) == 0)
            compiler->main_fn_idx = symbols(compiler)[fn_decl->id].idx;

        struct FnObj *fn = (struct FnObj*)alloc_vm_obj(vm, sizeof(struct FnObj));
        char *name = allocate((fn_decl->base.span.length+1)*sizeof(char));
        strncpy(name, fn_span.start, fn_span.length);
        name[fn_span.length] = '\0';

        init_fn_obj(fn, name, fn_decl->arity);

        // TODO clean up
        u32 stack_pos = compiler->stack_pos;
        for (i32 i = 0; i < fn_decl->arity; i++) {
            symbols(compiler)[fn_decl->params[i].id].idx = compiler->stack_pos;
            compiler->stack_pos++;
        }
        compiler->fn = fn;
        compile_node(compiler, (struct Node*)fn_decl->body);
        emit_byte(cur_chunk(compiler), OP_NIL, fn_span.line);
        emit_byte(cur_chunk(compiler), OP_RETURN, fn_span.line);
        compiler->stack_pos = stack_pos;

        compiler->fn = script;
        emit_byte(cur_chunk(compiler), OP_GET_CONST, fn_span.line);
        emit_byte(cur_chunk(compiler), add_constant(&script->chunk, MK_OBJ((struct Obj*)fn)), fn_span.line);
        emit_byte(cur_chunk(compiler), OP_SET_GLOBAL, fn_span.line);
        // TODO handle > 256 globals
        emit_byte(cur_chunk(compiler), compiler->global_cnt, fn_span.line);
        emit_byte(cur_chunk(compiler), OP_POP, fn_span.line);
        compiler->global_cnt++;
    }
    return script;
}