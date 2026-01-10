#include "compile.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "parse.h"
#include "sema.h"
#include "value.h"
#include <stdlib.h>

static i32 emit_jump(CompileCtx &c, const OpCode op, const i32 line)
{
    const i32 offset = c.chunk().code().len();
    c.chunk().emit_byte(op, line);
    // skip two bytes for jump
    //      OP_JUMP
    //      hi
    //      lo
    c.chunk().emit_byte(0, line);
    c.chunk().emit_byte(0, line);
    return offset;
}

static void patch_jump(CompileCtx &c, const Span span, const i32 offset)
{
    // offset idx of the OP_JUMP instr
    // len is idx of to-be-executed instr
    const i32 jump = c.chunk().code().len() - (offset + 3);
    if (jump > ((1 << 16) - 1))
        c.errarr.push(ErrMsg{span, "jump too far"});
    c.chunk().code()[offset + 1] = (jump >> 8) & 0xff;
    c.chunk().code()[offset + 2] = jump & 0xff;
}

static void emit_constant(CompileCtx &c, const Value val, const i32 line)
{
    c.chunk().emit_byte(OP_GET_CONST, line);
    c.chunk().emit_byte(c.chunk().add_constant(val), line);
}

static void compile_node(CompileCtx &c, const Node &node);

static void compile_atom(CompileCtx &c, const AtomNode &node)
{
    const i32 line = node.span.line;
    // clang-format off
    switch(node.atom_tag) {
    case TOKEN_NULL:   c.chunk().emit_byte(OP_NULL, line); break;
    case TOKEN_TRUE:   c.chunk().emit_byte(OP_TRUE, line); break;
    case TOKEN_FALSE:  c.chunk().emit_byte(OP_FALSE, line); break;
    case TOKEN_NUMBER: emit_constant(c, MK_NUM(strtod(node.span.start, nullptr)), line); break;
    case TOKEN_STRING: emit_constant(c, MK_OBJ(alloc<StringObj>(c.vm, node.span)), line); break;
    default:           c.errarr.push(ErrMsg{node.span, "default case of compile_atom reached"});
    }
    // clang-format on
}

// given an identifier that is captured return what it's idx will be
// in the fn's capture array, or -1 if this fn does not capture it
static i32 resolve_capture(const FnDeclNode &node, const i32 id)
{
    // capture count cannot exceed MAX_LOCALS
    for (i32 j = 0; j < MAX_LOCALS; j++) {
        if (j < node.stack_capture_cnt && id == node.stack_captures[j])
            return j;
        if (j < node.parent_capture_cnt && id == node.parent_captures[j])
            return j + node.stack_capture_cnt;
    }
    // TODO should be unreachable instead of marking -1
    return -1;
}

static void compile_ident_get_or_set(CompileCtx &c, const i32 id, const bool get, const i32 line)
{
    const Ident ident = c.idarr[id];
    if (ident.depth == 0) {
        c.chunk().emit_byte(get ? OP_GET_GLOBAL : OP_SET_GLOBAL, line);
        c.chunk().emit_byte(ident.idx, line);
        return;
    }
    if (ident.flags & FLAG_CAPTURED) {
        const i32 captures_arr_idx = resolve_capture(*c.fn_node, id);
        // ptr to the value lives on the stack
        if (captures_arr_idx == -1) {
            c.chunk().emit_byte(get ? OP_GET_HEAPVAL : OP_SET_HEAPVAL, line);
            c.chunk().emit_byte(ident.idx, line);
            return;
        }
        c.chunk().emit_byte(get ? OP_GET_CAPTURED : OP_SET_CAPTURED, line);
        c.chunk().emit_byte(captures_arr_idx, line);
        return;
    }
    c.chunk().emit_byte(get ? OP_GET_LOCAL : OP_SET_LOCAL, line);
    c.chunk().emit_byte(ident.idx, line);
}

// ident get
static void compile_ident(CompileCtx &c, const IdentNode &node)
{
    compile_ident_get_or_set(c, node.id, true, node.span.line);
}

static void compile_list(CompileCtx &c, const ListNode &node)
{
    // TODO allow lists to be initialized with more than 256 elements
    // TODO handle lists being initialized with too many elements
    const i32 line = node.span.line;
    for (i32 i = 0; i < node.cnt; i++)
        compile_node(c, *node.items[i]);
    c.chunk().emit_byte(OP_LIST, line);
    c.chunk().emit_byte(node.cnt, line);
}

static void compile_unary(CompileCtx &c, const UnaryNode &node)
{
    const i32 line = node.span.line;
    compile_node(c, *node.rhs);
    if (node.op_tag == TOKEN_MINUS)
        c.chunk().emit_byte(OP_NEGATE, line);
    else if (node.op_tag == TOKEN_NOT)
        c.chunk().emit_byte(OP_NOT, line);
}

static void compile_binary(CompileCtx &c, const BinaryNode &node)
{
    const i32 line = node.span.line;
    const TokenTag op_tag = node.op_tag;
    if (op_tag == TOKEN_EQ) {
        if (node.lhs->tag == NODE_IDENT) {
            // ident set
            const auto &lhs = static_cast<const IdentNode &>(*node.lhs);
            compile_node(c, *node.rhs);
            compile_ident_get_or_set(c, lhs.id, false, lhs.span.line);
        } else if (node.lhs->tag == NODE_BINARY) {
            // list set
            const auto &lhs = static_cast<const BinaryNode &>(*node.lhs);
            // TODO assert lhs.op_tag == TOKEN_L_SQUARE
            compile_node(c, *lhs.lhs);
            compile_node(c, *lhs.rhs);
            compile_node(c, *node.rhs);
            c.chunk().emit_byte(OP_SET_SUBSCR, line);
        } else if (node.lhs->tag == NODE_PROPERTY) {
            // field set
            const auto &lhs = static_cast<const PropertyNode &>(*node.lhs);
            // TODO assert lhs.op_tag == TOKEN_DOT
            compile_node(c, *lhs.lhs);
            compile_node(c, *node.rhs);
            c.chunk().emit_byte(OP_SET_FIELD, line);
            c.chunk().emit_byte(c.chunk().add_constant(MK_OBJ(alloc<StringObj>(c.vm, lhs.sym))), line);
        }
    } else if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
        compile_node(c, *node.lhs);
        const i32 offset = emit_jump(c, op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE, line);
        c.chunk().emit_byte(OP_POP, line);
        compile_node(c, *node.rhs);
        patch_jump(c, node.span, offset);
    } else {
        compile_node(c, *node.lhs);
        compile_node(c, *node.rhs);
        // clang-format off
        switch (op_tag) {
        case TOKEN_PLUS:        c.chunk().emit_byte(OP_ADD, line); break;
        case TOKEN_MINUS:       c.chunk().emit_byte(OP_SUB, line); break;
        case TOKEN_STAR:        c.chunk().emit_byte(OP_MUL, line); break;
        case TOKEN_SLASH:       c.chunk().emit_byte(OP_DIV, line); break;
        case TOKEN_SLASH_SLASH: c.chunk().emit_byte(OP_FLOORDIV, line); break;
        case TOKEN_PERCENT:     c.chunk().emit_byte(OP_MOD, line); break;
        case TOKEN_LT:          c.chunk().emit_byte(OP_LT, line); break;
        case TOKEN_LEQ:         c.chunk().emit_byte(OP_LEQ, line); break;
        case TOKEN_GT:          c.chunk().emit_byte(OP_GT, line); break;
        case TOKEN_GEQ:         c.chunk().emit_byte(OP_GEQ, line); break;
        case TOKEN_EQEQ:        c.chunk().emit_byte(OP_EQEQ, line); break;
        case TOKEN_NEQ:         c.chunk().emit_byte(OP_NEQ, line); break;
        case TOKEN_L_SQUARE:    c.chunk().emit_byte(OP_GET_SUBSCR, line); break;
        default:                c.errarr.push(ErrMsg{node.span, "default case of compile_binary reached"});
        }
        // clang-format on
    }
}

static void compile_get_property(CompileCtx &c, const PropertyNode &node)
{
    const i32 line = node.span.line;
    compile_node(c, *node.lhs);
    StringObj *str = alloc<StringObj>(c.vm, String(node.sym));
    c.chunk().emit_byte(node.op_tag == TOKEN_DOT ? OP_GET_FIELD : OP_GET_METHOD, line);
    c.chunk().emit_byte(c.chunk().add_constant(MK_OBJ(str)), line);
}

static void compile_call(CompileCtx &c, const CallNode &node)
{
    const i32 line = node.span.line;
    compile_node(c, *node.lhs);
    for (i32 i = 0; i < node.arity; i++)
        compile_node(c, *node.args[i]);
    c.chunk().emit_byte(OP_CALL, line);
    c.chunk().emit_byte(node.arity, line);
    // since there may be any number of locals on the stack when the callee returns
    // the callee is responsible for cleanup and moving the return value
}

static void compile_block(CompileCtx &c, const BlockNode &node)
{
    const i32 line = node.span.line;
    for (i32 i = 0; i < node.cnt; i++)
        compile_node(c, *node.stmts[i]);
    // TODO handle more than 256 locals
    if (node.local_cnt == 1) {
        c.chunk().emit_byte(OP_POP, line);
    } else if (node.local_cnt > 1) {
        c.chunk().emit_byte(OP_POP_N, line);
        c.chunk().emit_byte(node.local_cnt, line);
    }
}

static void compile_if(CompileCtx &c, const IfNode &node)
{
    const i32 line = node.span.line;
    compile_node(c, *node.cond);
    //      OP_JUMP_IF_FALSE (jump 1)
    //      OP_POP
    //      thn block
    //      OP_JUMP          (jump 2)
    //      OP_POP           (destination of jump 1)
    //      els block
    //      ...              (destination of jump 2)
    const i32 offset1 = emit_jump(c, OP_JUMP_IF_FALSE, line);
    c.chunk().emit_byte(OP_POP, line);
    compile_block(c, *node.thn);
    const i32 offset2 = emit_jump(c, OP_JUMP, line);
    patch_jump(c, node.span, offset1);
    c.chunk().emit_byte(OP_POP, line);
    if (node.els)
        compile_block(c, *node.els);
    patch_jump(c, node.span, offset2);
}

static void compile_expr_stmt(CompileCtx &c, const ExprStmtNode &node)
{
    compile_node(c, *node.expr);
    c.chunk().emit_byte(OP_POP, node.span.line);
}

static void compile_return(CompileCtx &c, const ReturnNode &node)
{
    const i32 line = node.span.line;
    if (node.expr) {
        compile_node(c, *node.expr);
    } else if (c.idarr[c.fn_node->id].flags & FLAG_INIT) {
        c.chunk().emit_byte(OP_GET_LOCAL, line);
        c.chunk().emit_byte(0, line);
    } else {
        c.chunk().emit_byte(OP_NULL, line);
    }
    c.chunk().emit_byte(OP_RETURN, line);
}

static void compile_print(CompileCtx &c, const PrintNode &node)
{
    compile_node(c, *node.expr);
    c.chunk().emit_byte(OP_PRINT, node.span.line);
}

static void compile_var_decl(CompileCtx &c, const VarDeclNode &node)
{
    const i32 line = node.span.line;
    const Ident &ident = c.idarr[node.id];
    if (node.init)
        compile_node(c, *node.init);
    else
        c.chunk().emit_byte(OP_NULL, line);
    // move variable on heap if it is captured
    if (ident.flags & FLAG_CAPTURED) {
        c.chunk().emit_byte(OP_HEAPVAL, line);
        c.chunk().emit_byte(ident.idx, line);
    }
}

static void compile_fn_body(CompileCtx &c, const FnDeclNode &node)
{
    const i32 line = node.span.line;
    for (i32 i = 0; i < node.arity; i++) {
        const Ident &param = c.idarr[node.params[i].id];
        // move param on heap if it is captured
        if (param.flags & FLAG_CAPTURED) {
            c.chunk().emit_byte(OP_HEAPVAL, line);
            c.chunk().emit_byte(param.idx, line);
        }
    }
    compile_block(c, *node.body);
    if (c.idarr[node.id].flags & FLAG_INIT) {
        c.chunk().emit_byte(OP_GET_LOCAL, line);
        c.chunk().emit_byte(c.fn_node->arity - 1, line);
    } else {
        c.chunk().emit_byte(OP_NULL, line);
    }
    c.chunk().emit_byte(OP_RETURN, line);
    // disassemble_chunk(c.chunk(), c.fn->name.chars());
}

static void compile_fn_decl(CompileCtx &c, const FnDeclNode &node)
{
    const i32 line = node.span.line;
    const Ident &ident = c.idarr[node.id];

    FnObj *fn = alloc<FnObj>(c.vm, node.span, Chunk(), node.arity);
    FnObj *parent = c.fn;
    const FnDeclNode *parent_node = c.fn_node;
    c.fn = fn;
    c.fn_node = &node;
    compile_fn_body(c, node);
    c.fn = parent;
    c.fn_node = parent_node;

    emit_constant(c, MK_OBJ(fn), line);
    // wrap the fn in a closure
    c.chunk().emit_byte(OP_CLOSURE, line);
    c.chunk().emit_byte(node.stack_capture_cnt, line);
    c.chunk().emit_byte(node.parent_capture_cnt, line);

    for (i32 i = 0; i < node.stack_capture_cnt; i++)
        c.chunk().emit_byte(c.idarr[node.stack_captures[i]].idx, line);

    for (i32 i = 0; i < node.parent_capture_cnt; i++)
        // node->parent can't be nullptr
        c.chunk().emit_byte(resolve_capture(*node.parent, node.parent_captures[i]), line);

    // move closure on heap if it is captured
    if (ident.flags & FLAG_CAPTURED) {
        c.chunk().emit_byte(OP_HEAPVAL, line);
        c.chunk().emit_byte(ident.idx, line);
    }
}

static void compile_node(CompileCtx &c, const Node &node)
{
    // clang-format off
    switch (node.tag) {
    case NODE_ATOM:       compile_atom(c, static_cast<const AtomNode&>(node)); break;
    case NODE_LIST:       compile_list(c, static_cast<const ListNode&>(node)); break;
    case NODE_IDENT:      compile_ident(c, static_cast<const IdentNode&>(node)); break;
    case NODE_UNARY:      compile_unary(c, static_cast<const UnaryNode&>(node)); break;
    case NODE_BINARY:     compile_binary(c, static_cast<const BinaryNode&>(node)); break;
    case NODE_PROPERTY:   compile_get_property(c, static_cast<const PropertyNode&>(node)); break;
    case NODE_CALL:       compile_call(c, static_cast<const CallNode&>(node)); break;
    case NODE_VAR_DECL:   compile_var_decl(c, static_cast<const VarDeclNode&>(node)); break;
    case NODE_FN_DECL:    compile_fn_decl(c, static_cast<const FnDeclNode&>(node)); break;
    case NODE_EXPR_STMT:  compile_expr_stmt(c, static_cast<const ExprStmtNode&>(node)); break;
    case NODE_BLOCK:      compile_block(c, static_cast<const BlockNode&>(node)); break;        
    case NODE_IF:         compile_if(c, static_cast<const IfNode&>(node)); break;
    case NODE_RETURN:     compile_return(c, static_cast<const ReturnNode&>(node)); break;
    // TEMP remove when we add functions
    case NODE_PRINT:      compile_print(c, static_cast<const PrintNode&>(node)); break; 
    default:              {} // TODO ASSERT FALSE
    }
    // clang-format on
}

ClosureObj *CompileCtx::compile(VM &vm, const Dynarr<Ident> &idarr, const ModuleNode &node, Dynarr<ErrMsg> &errarr)
{
    CompileCtx c(vm, idarr, errarr);
    ClosureObj *main = nullptr;
    for (i32 i = 0; i < node.cnt; i++)
        vm.globals.push(MK_NULL);
    for (i32 i = 0; i < node.cnt; i++) {
        if (node.decls[i]->tag == NODE_FN_DECL) {
            const auto &fn_node = static_cast<const FnDeclNode &>(*node.decls[i]);
            FnObj *fn = alloc<FnObj>(c.vm, fn_node.span, Chunk(), fn_node.arity);
            c.fn_node = &fn_node;
            c.fn = fn;
            compile_fn_body(c, fn_node);
            ClosureObj *closure = alloc<ClosureObj>(c.vm, fn, 0);
            vm.globals[c.idarr[c.fn_node->id].idx] = MK_OBJ(closure);
            if (fn_node.span == "main")
                main = closure;
        } else {
            const auto &class_node = static_cast<const ClassDeclNode &>(*node.decls[i]);
            ClassObj *klass = alloc<ClassObj>(c.vm, class_node.span);
            for (i32 i = 0; i < class_node.cnt; i++) {
                const auto &fn_node = static_cast<const FnDeclNode &>(*class_node.methods[i]);
                FnObj *fn = alloc<FnObj>(c.vm, fn_node.span, Chunk(), fn_node.arity);
                c.fn_node = &fn_node;
                c.fn = fn;
                compile_fn_body(c, fn_node);
                ClosureObj *closure = alloc<ClosureObj>(c.vm, fn, 0);
                klass->methods.insert(*alloc<StringObj>(c.vm, fn_node.span), MK_OBJ(closure));
            }
            vm.globals[c.idarr[class_node.id].idx] = MK_OBJ(klass);
        }
    }
    return main;
}
