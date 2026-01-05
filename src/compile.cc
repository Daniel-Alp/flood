// #include <assert.h>
// #include <string.h>
// #include <stdlib.h>
// #include "compile.h"
// #include "chunk.h"
// #include "error.h"
// #include "scan.h"
// #include "sema.h"
// #include "symbol.h"
// #include "parse.h"

// // static struct Chunk *cur_chunk(struct Compiler *compiler) 
// // {
// //     return &compiler->fn->chunk;
// // }

// // static struct Symbol *symbols(struct Compiler *compiler)
// // {
// //     return compiler->sym_arr->symbols;
// // }

// static i32 emit_jump(CompileCtx &c, const OpCode op, const i32 line) 
// {
//     const i32 offset = c.chunk().len();
//     c.chunk().emit_byte(op, line);
//     // skip two bytes for jump
//     //      OP_JUMP
//     //      hi
//     //      lo
//     c.chunk().emit_byte(0, line);
//     c.chunk().emit_byte(0, line);
//     return offset;
// }

// static void patch_jump(CompileCtx &c, const Span span, const i32 offset) 
// {
//     // offset idx of the OP_JUMP instr
//     // len is idx of to-be-executed instr
//     const i32 jump = c.chunk().len() - (offset+3);
//     if (jump > ((1 << 16) - 1))
//         c.errarr.push(ErrMsg{span, "jump too far"});
//     c.chunk()[offset+1] = (jump >> 8) & 0xff;
//     c.chunk()[offset+2] = jump & 0xff; 
// }

// static void emit_constant(CompileCtx &c, const Value val, const i32 line)
// {
//     c.chunk().emit_byte(OP_GET_CONST, line);
//     c.chunk().emit_byte(c.chunk().add_constant(val), line);
// }

// static void compile_node(CompileCtx &c, const Node &node);

// static void compile_atom(CompileCtx &c, const AtomNode &node) 
// {
//     const i32 line = node.span.line;
//     switch(node.atom_tag) {
//     case TOKEN_NULL:   c.chunk().emit_byte(OP_NULL, line); break;
//     case TOKEN_TRUE:   c.chunk().emit_byte(OP_TRUE, line); break;
//     case TOKEN_FALSE:  c.chunk().emit_byte(OP_FALSE, line); break;
//     // FIXME!!!
//     case TOKEN_NUMBER: emit_constant(c, MK_NUM(strtod(node.span.start, nullptr)), line); break;
//     case TOKEN_STRING: emit_constant(c, MK_OBJ((struct Obj*)string_from_span(c->vm, node->base.span)), line); break; 
//     default:           c.errarr.push(ErrMsg{node.span, "default case of compile_atom reached"});
//     }
// }

// // given an identifier that is captured return what it's idx will be 
// // in the fn's capture array, or -1 if this fn does not capture it
// static i32 resolve_capture(const FnDeclNode &node, const i32 id)
// {
//     // capture count cannot exceed MAX_LOCALS
//     for (i32 j = 0; j < MAX_LOCALS; j++) {
//         if (j < node.stack_capture_cnt && id == node.stack_captures[j])
//             return j;
//         if (j < node.parent_capture_cnt && id == node.parent_captures[j])
//             return j + node.stack_capture_cnt;
//     }
//     // TODO should be unreachable instead of marking -1
//     return -1;
// }

// static void compile_ident_get_or_set(CompileCtx &c, const i32 id, const bool get, const i32 line)
// {
//     const Symbol sym = c.symarr[id];
//     // special case if a function is referencing itself we can do OP_GET_LOCAL 0 or OP_SET_LOCAL 0
//     if (id == c.fn_node->id) {
//         c.chunk().emit_byte(get ? OP_GET_LOCAL : OP_SET_LOCAL, line);
//         c.chunk().emit_byte(0, line);
//         return;
//     }
//     if (sym.depth == 0) {
//         c.chunk().emit_byte(get ? OP_GET_GLOBAL : OP_SET_GLOBAL, line);
//         c.chunk().emit_byte(sym.idx, line);
//         return;
//     }
//     if (sym.flags & FLAG_CAPTURED) {
//         const i32 captures_arr_idx = resolve_capture(*c.fn_node, id);
//         // ptr to the value lives on the stack
//         if (captures_arr_idx == -1) {
//             c.chunk().emit_byte(get ? OP_GET_HEAPVAL : OP_SET_HEAPVAL, line);
//             c.chunk().emit_byte(sym.idx, line);
//             return;
//         }
//         c.chunk().emit_byte(get ? OP_GET_CAPTURED : OP_SET_CAPTURED, line);
//         c.chunk().emit_byte(captures_arr_idx, line);
//         return;
//     } 
//     c.chunk().emit_byte(get ? OP_GET_LOCAL : OP_SET_LOCAL, line);
//     c.chunk().emit_byte(sym.idx, line);
// }

// // ident get
// static void compile_ident(CompileCtx &c, const IdentNode &node) 
// {
//     compile_ident_get_or_set(c, node.id, true, node.span.line);
// }

// static void compile_list(CompileCtx &c, const ListNode &node) 
// {
//     // TODO allow lists to be initialized with more than 256 elements
//     // TODO handle lists being initialized with too many elements
//     const i32 line = node.span.line;
//     for (i32 i = 0; i < node.cnt; i++)
//         compile_node(c, *node.items[i]);    
//     c.chunk().emit_byte(OP_LIST, line);
//     c.chunk().emit_byte(node.cnt, line);
// }

// static void compile_unary(CompileCtx &c, const UnaryNode &node) 
// {
//     const i32 line = node.span.line;
//     compile_node(c, *node.rhs);
//     if (node.op_tag == TOKEN_MINUS)
//         c.chunk().emit_byte(OP_NEGATE, line);
//     else if (node.op_tag == TOKEN_NOT)
//         c.chunk().emit_byte(OP_NOT, line);
// }

// static void compile_binary(CompileCtx &c, const BinaryNode &node) 
// {
//     const i32 line = node.span.line;
//     const TokenTag op_tag = node.op_tag;
//     if (op_tag == TOKEN_EQ) {
//         if (node.lhs->tag == NODE_IDENT) {
//             // ident set
//             const auto& lhs = static_cast<const IdentNode&>(*node.lhs);
//             compile_node(c, *node.rhs);
//             compile_ident_get_or_set(c, lhs.id, false, lhs.span.line);
//         } else if (node.lhs->tag == NODE_BINARY) {
//             // list set
//             const auto& lhs = static_cast<const BinaryNode&>(*node.lhs);
//             // TODO assert lhs.op_tag == TOKEN_L_SQUARE
//             compile_node(c, *lhs.lhs);
//             compile_node(c, *lhs.rhs);
//             compile_node(c, *node.rhs);
//             c.chunk().emit_byte(OP_SET_SUBSCR, line);
//         } else if (node.lhs->tag == NODE_PROPERTY) {
//             // field set
//             const auto& lhs = static_cast<const PropertyNode&>(*node.lhs);
//             // TODO assert lhs.op_tag == TOKEN_DOT
//             compile_node(c, *lhs.lhs);
//             compile_node(c, *node.rhs);
//             c.chunk().emit_byte(OP_SET_FIELD, line);
//             // FIXME!!!
//             // struct StringObj *str = string_from_span(c->vm, ((struct PropertyNode*)node->lhs)->sym);
//             // emit_byte(cur_chunk(c), add_constant(cur_chunk(c), MK_OBJ((struct Obj*)str)), line);
//         }
//     } else if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
//         compile_node(c, *node.lhs);
//         const i32 offset = emit_jump(c, op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE, line);
//         c.chunk().emit_byte(OP_POP, line);
//         compile_node(c, *node.rhs);
//         patch_jump(c, node.span, offset);
//     } else {
//         compile_node(c, *node.lhs);
//         compile_node(c, *node.rhs);
//         switch (op_tag) {
//         case TOKEN_PLUS:        c.chunk().emit_byte(OP_ADD, line); break;
//         case TOKEN_MINUS:       c.chunk().emit_byte(OP_SUB, line); break;
//         case TOKEN_STAR:        c.chunk().emit_byte(OP_MUL, line); break;
//         case TOKEN_SLASH:       c.chunk().emit_byte(OP_DIV, line); break;
//         case TOKEN_SLASH_SLASH: c.chunk().emit_byte(OP_FLOORDIV, line); break;
//         case TOKEN_PERCENT:     c.chunk().emit_byte(OP_MOD, line); break;
//         case TOKEN_LT:          c.chunk().emit_byte(OP_LT, line); break;
//         case TOKEN_LEQ:         c.chunk().emit_byte(OP_LEQ, line); break;
//         case TOKEN_GT:          c.chunk().emit_byte(OP_GT, line); break;
//         case TOKEN_GEQ:         c.chunk().emit_byte(OP_GEQ, line); break;
//         case TOKEN_EQEQ:        c.chunk().emit_byte(OP_EQEQ, line); break;
//         case TOKEN_NEQ:         c.chunk().emit_byte(OP_NEQ, line); break;
//         case TOKEN_L_SQUARE:    c.chunk().emit_byte(OP_GET_SUBSCR, line); break;
//         default:                c.errarr.push(ErrMsg{node.span, "default case of compile_binary reached"});
//         }
//     }
// }

// static void compile_get_property(CompileCtx &c, const PropertyNode &node)
// {
//     const i32 line = node.span.line;
//     compile_node(c, *node.lhs);
//     // FIXME!!!
//     // struct StringObj *str = string_from_span(c->vm, node->sym);
//     c.chunk().emit_byte(node.op_tag == TOKEN_DOT ? OP_GET_FIELD : OP_GET_METHOD, line); 
//     // emit_byte(cur_chunk(c), add_constant(cur_chunk(c), MK_OBJ((struct Obj*)str)), line);
// }

// static void compile_call(CompileCtx &c, const CallNode &node) 
// {
//     const i32 line = node.span.line;
//     compile_node(c, *node.lhs);
//     for (i32 i = 0; i < node.arity; i++)
//         compile_node(c, *node.args[i]);
//     c.chunk().emit_byte(OP_CALL, line);
//     c.chunk().emit_byte(node.arity, line);
//     // since there may be any number of locals on the stack when the callee returns
//     // the callee is responsible for cleanup and moving the return value
// }

// static void compile_block(CompileCtx &c, const BlockNode &node) 
// {
//     const i32 line = node.span.line;
//     const i32 local_cnt = c.fn_local_cnt;
//     for (i32 i = 0; i < node.cnt; i++)
//         compile_node(c, *node.stmts[i]);
//     // TODO handle more than 256 locals 
//     const i32 n = c.fn_local_cnt - local_cnt;
//     if (n == 1) {
//         c.chunk().emit_byte(OP_POP, line);
//     } else if (n > 1) {
//         c.chunk().emit_byte(OP_POP_N, line);
//         c.chunk().emit_byte(n, line);
//     }
//     c.fn_local_cnt = local_cnt;
// }

// static void compile_if(CompileCtx &c, const IfNode &node) 
// {
//     const i32 line = node.span.line;
//     compile_node(c, *node.cond);
//     //      OP_JUMP_IF_FALSE (jump 1)
//     //      OP_POP           
//     //      thn block              
//     //      OP_JUMP          (jump 2)
//     //      OP_POP           (destination of jump 1)
//     //      els block        
//     //      ...              (destination of jump 2)
//     const i32 offset1 = emit_jump(c, OP_JUMP_IF_FALSE, line);
//     c.chunk().emit_byte(OP_POP, line);
//     compile_block(c, *node.thn);
//     const i32 offset2 = emit_jump(c, OP_JUMP, line);
//     patch_jump(c, node.span, offset1);
//     c.chunk().emit_byte(OP_POP, line);
//     if (node.els)
//         compile_block(c, *node.els);
//     patch_jump(c, node.span, offset2);
// }

// static void compile_expr_stmt(CompileCtx &c, const ExprStmtNode &node) 
// {
//     compile_node(c, *node.expr);
//     c.chunk().emit_byte(OP_POP, node.span.line);
// }

// static void compile_return(CompileCtx &c, const ReturnNode &node) 
// {
//     const i32 line = node.span.line;
//     if (node.expr) {
//         compile_node(c, *node.expr);
//     } else if (c.symarr[c.fn_node->id].flags & FLAG_INIT) {
//         // `init` method implicitly returns `self` which is at bp[arity]
//         c.chunk().emit_byte(OP_GET_LOCAL, line);
//         c.chunk().emit_byte(c.fn_node->arity, line);
//     } else {
//         c.chunk().emit_byte(OP_NULL, line);
//     }
//     c.chunk().emit_byte(OP_RETURN, line);
// }

// static void compile_print(CompileCtx &c, const PrintNode &node) 
// {
//     compile_node(c, *node.expr);
//     c.chunk().emit_byte(OP_PRINT, node.span.line);
// }

// static void compile_var_decl(CompileCtx &c, const VarDeclNode &node) 
// {
//     const i32 line = node.span.line;
//     // TODO var decls for class members 
//     Symbol &sym = c.symarr[node.id];
//     sym.idx = c.fn_local_cnt;
//     c.fn_local_cnt++;
//     if (node.init)
//         compile_node(c, *node.init);
//     else 
//         c.chunk().emit_byte(OP_NULL, line);
//     // move variable on heap if it is captured
//     if (sym.flags & FLAG_CAPTURED) {
//         c.chunk().emit_byte(OP_HEAPVAL, line);
//         c.chunk().emit_byte(sym.idx, line);
//     }
// }

// static void compile_fn_decl(CompileCtx &c, const FnDeclNode &node)
// {
//     // FIXME!!!
//     // const i32 line = node.span.line;
//     // Symbol &sym = c.symarr[node.id];
//     // // top level functions are hoisted
//     // if (sym.idx == -1) {
//     //     sym.idx = c.fn_local_cnt;
//     //     c.fn_local_cnt++;
//     // }

//     // struct FnObj* fn = (struct FnObj*)alloc_vm_obj(c->vm, sizeof(struct FnObj));
//     // init_fn_obj(fn, string_from_span(c->vm, node->base.span), node->arity);

//     // // push state and enter the fn
//     // i32 parent_local_cnt = c->fn_local_cnt;
//     // struct FnObj* parent = c->fn; 
//     // struct FnDeclNode *parent_node = c->fn_node;
//     // c->fn_local_cnt = 1;
//     // c->fn = fn;
//     // c->fn_node = node;
//     // for (i32 i = 0; i < node->arity; i++) {
//     //     struct Symbol *param_sym = &symbols(c)[node->params[i].id];
//     //     param_sym->idx = c->fn_local_cnt;
//     //     c->fn_local_cnt++;
//     //     // move param on heap if it is captured
//     //     if (param_sym->flags & FLAG_CAPTURED) {
//     //         emit_byte(cur_chunk(c), OP_HEAPVAL, line);
//     //         emit_byte(cur_chunk(c), param_sym->idx, line);
//     //     }
//     // }
//     // compile_block(c, node->body);
//     // if (sym->flags & FLAG_INIT) {
//     //     // `init` method implicitly returns `self` which is at bp[arity]
//     //     emit_byte(cur_chunk(c), OP_GET_LOCAL, node->base.span.line);
//     //     emit_byte(cur_chunk(c), c->fn_node->arity, node->base.span.line);
//     // } else {
//     //     emit_byte(cur_chunk(c), OP_NULL, line);
//     // }
//     // emit_byte(cur_chunk(c), OP_RETURN, line);
//     // // disassemble_chunk(cur_chunk(compiler), compiler->fn->name->chars);

//     // // pop state and exit the fn
//     // c->fn_local_cnt = parent_local_cnt;
//     // c->fn = parent;
//     // c->fn_node = parent_node;
//     // emit_constant(c, MK_OBJ((struct Obj*)fn), line);
//     // // wrap the fn in a closure
//     // emit_byte(cur_chunk(c), OP_CLOSURE, line);
//     // emit_byte(cur_chunk(c), node->stack_capture_cnt, line);
//     // emit_byte(cur_chunk(c), node->parent_capture_cnt, line);
//     // for (i32 i = 0; i < node->stack_capture_cnt; i++) {
//     //     i32 id = node->stack_captures[i];
//     //     emit_byte(cur_chunk(c), symbols(c)[id].idx, line);
//     // }
//     // for (i32 i = 0; i < node->parent_capture_cnt; i++) {
//     //     i32 id = node->parent_captures[i];
//     //     // node->parent can't be NULL
//     //     emit_byte(cur_chunk(c), resolve_capture(node->parent, id), line);
//     // }
//     // // move closure on heap if it is captured
//     // if (sym->flags & FLAG_CAPTURED) {
//     //     emit_byte(cur_chunk(c), OP_HEAPVAL, line);
//     //     emit_byte(cur_chunk(c), sym->idx, line);
//     // }
// }

// static void compile_class_decl(CompileCtx &c, const ClassDeclNode &node)
// {
//     const i32 line = node.span.line;
//     // why am I determining this in the semantic phase?
//     Symbol &sym = c.symarr[node.id];
//     // top level functions are hoisted
//     if (sym.idx == -1) {
//         sym.idx = c.fn_local_cnt;
//         c.fn_local_cnt++;
//     }

//     c.chunk().emit_byte(OP_CLASS, line);
//     // FIXME!!!
//     // struct StringObj *str = string_from_span(c->vm, node->base.span);
//     // emit_byte(cur_chunk(c), add_constant(cur_chunk(c), MK_OBJ((struct Obj*)str)), line);

//     for (i32 i = 0; i < node.cnt; i++) {
//         compile_fn_decl(c, *node.methods[i]);
//         c.chunk().emit_byte(OP_GET_METHOD, line);
//     }
// }

// static void compile_node(CompileCtx &c, const Node &node) 
// {
//     switch (node.tag) {
//     case NODE_ATOM:       compile_atom(c, static_cast<const AtomNode&>(node)); break;
//     case NODE_LIST:       compile_list(c, static_cast<const ListNode&>(node)); break;
//     case NODE_IDENT:      compile_ident(c, static_cast<const IdentNode&>(node)); break;
//     case NODE_UNARY:      compile_unary(c, static_cast<const UnaryNode&>(node)); break;
//     case NODE_BINARY:     compile_binary(c, static_cast<const BinaryNode&>(node)); break;
//     case NODE_PROPERTY:   compile_get_property(c, static_cast<const PropertyNode&>(node)); break;
//     case NODE_CALL:       compile_call(c, static_cast<const CallNode&>(node)); break;
//     case NODE_VAR_DECL:   compile_var_decl(c, static_cast<const VarDeclNode&>(node)); break;
//     case NODE_FN_DECL:    compile_fn_decl(c, static_cast<const FnDeclNode&>(node)); break;
//     case NODE_CLASS_DECL: compile_class_decl(c, static_cast<const ClassDeclNode&>(node)); break;
//     case NODE_EXPR_STMT:  compile_expr_stmt(c, static_cast<const ExprStmtNode&>(node)); break;
//     case NODE_BLOCK:      compile_block(c, static_cast<const BlockNode&>(node)); break;        
//     case NODE_IF:         compile_if(c, static_cast<const IfNode&>(node)); break;
//     case NODE_RETURN:     compile_return(c, static_cast<const ReturnNode&>(node)); break;
//     // TEMP remove when we add functions
//     case NODE_PRINT:      compile_print(c, static_cast<const PrintNode&>(node)); break; 
//     }
// }

// // // TODO this isn't important right now but it's weird 
// // // that the compiler takes some arguments such as sym arr upon initialization 
// // // and others like VM each time you call compile_file
// // // later I'll probably have it so that the compiler owns its errlist but everything else 
// // // is passed in as args. This way you can use a single compiler struct for however many VMs you like.
// // void init_compiler(struct CompileCtx *compiler, struct SymArr *arr) 
// // {
// //     init_errarr(&compiler->errarr);
// //     compiler->main_fn_idx = -1;
// //     compiler->sym_arr = arr;
// //     compiler->vm = NULL;
// // }

// // void release_compiler(struct CompileCtx *compiler)
// // {
// //     release_errarr(&compiler->errarr);
// //     compiler->main_fn_idx = -1;
// //     // compiler does not own these
// //     compiler->sym_arr = NULL;
// //     compiler->vm = NULL;
// // }

// ClosureObj &CompileCtx::compile(VM &vm, const Dynarr<Symbol> &symarr, const Node &node, Dynarr<ErrMsg> &errarr) 
// {    
//     CompileCtx c(vm, symarr, node, errarr);

//     // // TODO I should probably be clearing the errorlist each time
//     // // have to clear other things in the compiler as well each time we compile
//     // compiler->vm = vm;

//     // struct FnObj *top_fn = (struct FnObj*)alloc_vm_obj(vm, sizeof(struct FnObj));
//     // init_fn_obj(top_fn, string_from_c_str(vm, "script"), 0);

//     // struct ClosureObj *top_closure = (struct ClosureObj*)alloc_vm_obj(vm, sizeof(struct ClosureObj));
//     // init_closure_obj(top_closure, top_fn, 0);

//     // compiler->fn_node = node;
//     // compiler->fn = top_fn;
//     // compiler->fn_local_cnt = 0;

//     // for (i32 i = 0; i < node->body->cnt; i++) {
//     //     // for top-level functions and classes we set their index before we compile them, hosting them up
//     //     if (node->body->stmts[i]->tag == NODE_FN_DECL) {
//     //         struct FnDeclNode *fn_node = (struct FnDeclNode*)node->body->stmts[i];
//     //         symbols(compiler)[fn_node->id].idx = compiler->fn_local_cnt;
//     //         struct Span fn_span = fn_node->base.span;
//     //         if (fn_span.len == 4 && strncmp(fn_span.start, "main", 4) == 0)
//     //             compiler->main_fn_idx = compiler->fn_local_cnt;
//     //     } else {
//     //         struct ClassDeclNode *class_node = (struct ClassDeclNode*)node->body->stmts[i];
//     //         symbols(compiler)[class_node->id].idx = compiler->fn_local_cnt;   
//     //     }
//     //     compiler->fn_local_cnt++;
//     // }
//     // for (i32 i = 0; i < node->body->cnt; i++) {
//     //     struct Node *stmt = node->body->stmts[i];
//     //     if (stmt->tag == NODE_FN_DECL)
//     //         compile_fn_decl(compiler, (struct FnDeclNode*)stmt);
//     //     else
//     //         compile_class_decl(compiler, (struct ClassDeclNode*)stmt);
//     //     i32 line = stmt->span.line;
//     //     emit_byte(cur_chunk(compiler), OP_SET_GLOBAL, line);
//     //     emit_byte(cur_chunk(compiler), i, line);
//     //     emit_byte(cur_chunk(compiler), OP_POP, line);
//     // }

//     // disassemble_chunk(cur_chunk(compiler), compiler->fn->name->chars);

//     return top_closure;
// }
