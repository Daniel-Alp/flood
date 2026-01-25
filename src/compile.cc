// #include "compile.h"
// #include "ast.h"
// #include "chunk.h"
// #include "object.h"
// #include <stdlib.h>

// static u8 desugar_assign(const TokenTag tag)
// {
//     switch (tag) {
//     case TOKEN_PLUS_EQ: return OP_ADD;
//     case TOKEN_MINUS_EQ: return OP_SUB;
//     case TOKEN_STAR_EQ: return OP_MUL;
//     case TOKEN_SLASH_EQ: return OP_DIV;
//     case TOKEN_SLASH_SLASH_EQ: return OP_FLOORDIV;
//     case TOKEN_PERCENT_EQ: return OP_MOD;
//     default: return 0;
//     }
// }

// struct Compiler final : AstVisitor {
//     FnObj *fn;
//     FnDeclNode *fn_node;
//     VM &vm;
//     Dynarr<ErrMsg> &errarr;
//     Compiler(VM &vm, Dynarr<ErrMsg> &errarr) : fn(nullptr), fn_node(nullptr), vm(vm), errarr(errarr) {}

//     Chunk &chunk()
//     {
//         return fn->chunk;
//     }

//     i32 emit_jump(const OpCode op, const i32 line)
//     {
//         const i32 offset = chunk().code().len();
//         chunk().emit_byte(op, line);
//         // skip two bytes for jump
//         //      OP_JUMP
//         //      hi
//         //      lo
//         chunk().emit_byte(0, line);
//         chunk().emit_byte(0, line);
//         return offset;
//     }

//     void patch_jump(const Span span, const i32 offset)
//     {
//         // offset idx of the OP_JUMP instr
//         // len is idx of to-be-executed instr
//         const i32 jump = chunk().code().len() - (offset + 3);
//         if (jump > ((1 << 16) - 1))
//             errarr.push(ErrMsg{span, "jump too far"});
//         chunk().code()[offset + 1] = (jump >> 8) & 0xff;
//         chunk().code()[offset + 2] = jump & 0xff;
//     }

//     void emit_constant(const Value val, const i32 line)
//     {
//         chunk().emit_byte(OP_GET_CONST, line);
//         chunk().emit_byte(chunk().add_constant(val), line);
//     }

//     void visit_atom(AtomNode &node) override
//     {
//         const i32 line = node.span.line;
//         // clang-format off
//         switch(node.atom_tag) {
//         case TOKEN_NULL:   chunk().emit_byte(OP_NULL, line); break;
//         case TOKEN_TRUE:   chunk().emit_byte(OP_TRUE, line); break;
//         case TOKEN_FALSE:  chunk().emit_byte(OP_FALSE, line); break;
//         case TOKEN_NUMBER: emit_constant(MK_NUM(strtod(node.span.start, nullptr)), line); break;
//         case TOKEN_STRING: emit_constant(MK_OBJ(alloc<StringObj>(vm, node.span)), line); break;
//         default:           errarr.push(ErrMsg{node.span, "default case of compile_atom reached"});
//         }
//         // clang-format on
//     }

//     void emit_ident_get_or_set(IdentNode &node, const bool get)
//     {
//         // clang-format off
//         const u8 op = node.loc.tag == LOC_GLOBAL          ? get ? OP_GET_GLOBAL : OP_SET_GLOBAL
//                       : node.loc.tag == LOC_LOCAL         ? get ? OP_GET_LOCAL : OP_SET_LOCAL
//                       : node.loc.tag == LOC_STACK_HEAPVAL ? get ? OP_GET_HEAPVAL : OP_SET_HEAPVAL
//                                                           : get ? OP_GET_CAPTURED : OP_SET_CAPTURED;
//         // clang-format on
//         chunk().emit_byte(op, node.span.line);
//         chunk().emit_byte(node.loc.idx, node.span.line);
//     }

//     void visit_ident(IdentNode &node) override
//     {
//         emit_ident_get_or_set(node, true);
//     }

//     void visit_list(ListNode &node) override
//     {
//         // TODO allow lists to be initialized with more than 256 elements
//         // TODO handle lists being initialized with too many elements
//         const i32 line = node.span.line;
//         for (i32 i = 0; i < node.cnt; i++)
//             visit_expr(*node.items[i]);
//         chunk().emit_byte(OP_LIST, line);
//         chunk().emit_byte(node.cnt, line);
//     }

//     void visit_unary(UnaryNode &node) override
//     {
//         const i32 line = node.span.line;
//         visit_expr(*node.rhs);
//         if (node.op_tag == TOKEN_MINUS)
//             chunk().emit_byte(OP_NEGATE, line);
//         else if (node.op_tag == TOKEN_NOT)
//             chunk().emit_byte(OP_NOT, line);
//     }

//     void visit_binary(BinaryNode &node) override
//     {
//         const i32 line = node.span.line;
//         const TokenTag op_tag = node.op_tag;
//         if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
//             visit_expr(*node.lhs);
//             const i32 offset = emit_jump(op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE, line);
//             chunk().emit_byte(OP_POP, line);
//             visit_expr(*node.rhs);
//             patch_jump(node.span, offset);
//             return;
//         }
//         visit_expr(*node.lhs);
//         visit_expr(*node.rhs);
//         // clang-format off
//         switch (op_tag) {
//         case TOKEN_PLUS:        chunk().emit_byte(OP_ADD, line); break;
//         case TOKEN_MINUS:       chunk().emit_byte(OP_SUB, line); break;
//         case TOKEN_STAR:        chunk().emit_byte(OP_MUL, line); break;
//         case TOKEN_SLASH:       chunk().emit_byte(OP_DIV, line); break;
//         case TOKEN_SLASH_SLASH: chunk().emit_byte(OP_FLOORDIV, line); break;
//         case TOKEN_PERCENT:     chunk().emit_byte(OP_MOD, line); break;
//         case TOKEN_LT:          chunk().emit_byte(OP_LT, line); break;
//         case TOKEN_LEQ:         chunk().emit_byte(OP_LEQ, line); break;
//         case TOKEN_GT:          chunk().emit_byte(OP_GT, line); break;
//         case TOKEN_GEQ:         chunk().emit_byte(OP_GEQ, line); break;
//         case TOKEN_EQEQ:        chunk().emit_byte(OP_EQEQ, line); break;
//         case TOKEN_NEQ:         chunk().emit_byte(OP_NEQ, line); break;
//         }
//         // clang-format on
//     }

//     void visit_subscr(SubscrNode &node) override
//     {
//         const i32 line = node.span.line;
//         visit_expr(*node.lhs);
//         visit_expr(*node.rhs);
//         chunk().emit_byte(OP_GET_SUBSCR, line);
//     }

//     void visit_assign(AssignNode &node) override
//     {
//         const i32 line = node.span.line;
//         // handle `+=`, `-=`, `*=`, `/=`, `//=`, `%=`
//         const TokenTag op_tag = node.op_tag;
//         if (op_tag != TOKEN_EQ)
//             visit_expr(*node.lhs);
//         visit_expr(*node.rhs);
//         if (op_tag != TOKEN_EQ)
//             chunk().emit_byte(desugar_assign(op_tag), line);

//         if (node.lhs->tag == NODE_IDENT) {
//             // ident set
//             emit_ident_get_or_set(static_cast<IdentNode &>(*node.lhs), false);
//         } else if (node.lhs->tag == NODE_SUBSCR) {
//             // subscr set
//             const auto &lhs = static_cast<const SubscrNode &>(*node.lhs);
//             visit_expr(*lhs.lhs);
//             visit_expr(*lhs.rhs);
//             chunk().emit_byte(OP_SET_SUBSCR, line);
//         } else if (node.lhs->tag == NODE_SELECTOR) {
//             // field set
//             const auto &lhs = static_cast<const SelectorNode &>(*node.lhs);
//             visit_expr(*lhs.lhs);
//             chunk().emit_byte(OP_SET_FIELD, line);
//             chunk().emit_byte(chunk().add_constant(MK_OBJ(alloc<StringObj>(vm, lhs.sym))), line);
//         }
//     }

//     void visit_selector(SelectorNode &node) override
//     {
//         const i32 line = node.span.line;
//         visit_expr(*node.lhs);
//         StringObj *str = alloc<StringObj>(vm, String(node.sym));
//         chunk().emit_byte(node.op_tag == TOKEN_DOT ? OP_GET_FIELD : OP_GET_METHOD, line);
//         chunk().emit_byte(chunk().add_constant(MK_OBJ(str)), line);
//     }

//     void compile_call(CallNode &node)
//     {
//         const i32 line = node.span.line;
//         visit_expr(*node.lhs);
//         for (i32 i = 0; i < node.arity; i++)
//             visit_expr(*node.args[i]);
//         chunk().emit_byte(OP_CALL, line);
//         chunk().emit_byte(node.arity, line);
//         // since there may be any number of locals on the stack when the callee returns
//         // the callee is responsible for cleanup and moving the return value
//     }

//     void visit_if(IfNode &node) override
//     {
//         const i32 line = node.span.line;
//         visit_expr(*node.cond);
//         //      OP_JUMP_IF_FALSE (jump 1)
//         //      OP_POP
//         //      thn block
//         //      OP_JUMP          (jump 2)
//         //      OP_POP           (destination of jump 1)
//         //      els block
//         //      ...              (destination of jump 2)
//         const i32 offset1 = emit_jump(OP_JUMP_IF_FALSE, line);
//         chunk().emit_byte(OP_POP, line);
//         visit_block(*node.thn);
//         const i32 offset2 = emit_jump(OP_JUMP, line);
//         patch_jump(node.span, offset1);
//         chunk().emit_byte(OP_POP, line);
//         if (node.els)
//             visit_block(*node.els);
//         patch_jump(node.span, offset2);
//     }

//     void visit_expr_stmt(ExprStmtNode &node) override
//     {
//         visit_expr(*node.expr);
//         chunk().emit_byte(OP_POP, node.span.line);
//     }

//     void visit_return(ReturnNode &node) override
//     {
//         const i32 line = node.span.line;
//         if (node.expr) {
//             visit_expr(*node.expr);
//         } else if (fn_node->flags & FLAG_INIT) {
//             chunk().emit_byte(OP_GET_LOCAL, line);
//             chunk().emit_byte(fn_node->arity - 1, line);
//         } else {
//             chunk().emit_byte(OP_NULL, line);
//         }
//         chunk().emit_byte(OP_RETURN, line);
//     }

//     void visit_print(PrintNode &node) override
//     {
//         visit_expr(*node.expr);
//         chunk().emit_byte(OP_PRINT, node.span.line);
//     }

//     void visit_var_decl(VarDeclNode &node) override
//     {
//         const i32 line = node.span.line;
//         if (node.init)
//             visit_expr(*node.init);
//         else
//             chunk().emit_byte(OP_NULL, line);
//         // move variable on heap if it is captured
//         if (node.flags & FLAG_CAPTURED)
//             chunk().emit_byte(OP_HEAPVAL, line);
//     }

//     void compile_fn_body(FnDeclNode &node)
//     {
//         const i32 line = node.span.line;
//         for (i32 i = 0; i < node.arity; i++) {
//             // move param on heap if it is captured
//             if (node.params[i].flags & FLAG_CAPTURED) {
//                 chunk().emit_byte(OP_HEAPVAL_2, line);
//                 chunk().emit_byte(i, line);
//             }
//         }
//         visit_block(*node.body);
//         if (node.flags & FLAG_INIT) {
//             chunk().emit_byte(OP_GET_LOCAL, line);
//             chunk().emit_byte(node.arity - 1, line);
//         } else {
//             chunk().emit_byte(OP_NULL, line);
//         }
//         chunk().emit_byte(OP_RETURN, line);
//         // disassemble_chunk(c.chunk(), c.fn->name->str.chars());
//     }

//     void visit_fn_decl(FnDeclNode &node) override
//     {
//         const i32 line = node.span.line;

//         // FnObj *fn = alloc<FnObj>(c.vm, alloc<StringObj>(vm, node.span), Chunk(), node.arity);
//         // FnObj *parent = c.fn;
//         // const FnDeclNode *parent_node = c.fn_node;
//         // c.fn = fn;
//         // c.fn_node = &node;
//         // compile_fn_body(c, node);
//         // c.fn = parent;
//         // c.fn_node = parent_node;

//         // emit_constant(c, MK_OBJ(fn), line);
//         // // wrap the fn in a closure
//         // c.chunk().emit_byte(OP_CLOSURE, line);
//         // c.chunk().emit_byte(node.stack_capture_cnt, line);
//         // c.chunk().emit_byte(node.parent_capture_cnt, line);

//         // // for (i32 i = 0; i < node.stack_capture_cnt; i++)
//         // //     c.chunk().emit_byte(c.idarr[node.stack_captures[i]].idx, line);

//         // // for (i32 i = 0; i < node.parent_capture_cnt; i++)
//         // //     // node->parent can't be nullptr
//         // //     c.chunk().emit_byte(resolve_capture(*node.parent, node.parent_captures[i]), line);

//         // // move closure on heap if it is captured
//         // if (ident.flags & FLAG_CAPTURED) {
//         //     c.chunk().emit_byte(OP_HEAPVAL, line);
//         //     c.chunk().emit_byte(ident.idx, line);
//         // }
//     }

//     // TODO need to know if fn_body
//     void visit_block(BlockNode &node) override
//     {
//         // const i32 line = node.span.line;
//         // for (i32 i = 0; i < node.cnt; i++)
//         //     visit_stmt(*node.stmts[i]);
//         // // don't pop locals in a function body, the return will pop locals
//         // if (fn_body) // FIXME
//         //     return;
//         // // TODO handle more than 256 locals
//         // if (node.local_cnt == 1) {
//         //     chunk().emit_byte(OP_POP, line);
//         // } else if (node.local_cnt > 1) {
//         //     chunk().emit_byte(OP_POP_N, line);
//         //     chunk().emit_byte(node.local_cnt, line);
//         // }
//     }

//     ClosureObj *visit(ModuleNode &node)
//     {
//         ClosureObj *main = nullptr;
//         for (i32 i = 0; i < node.cnt; i++)
//             vm.globals.push(MK_NULL);
//         for (i32 i = 0; i < node.cnt; i++) {
//             // if (node.decls[i]->tag == NODE_FN_DECL) {
//             //     const auto &fn_node = static_cast<const FnDeclNode &>(*node.decls[i]);
//             //     FnObj *fn = alloc<FnObj>(c.vm, alloc<StringObj>(c.vm, fn_node.span), Chunk(), fn_node.arity);
//             //     c.fn_node = &fn_node;
//             //     c.fn = fn;
//             //     compile_fn_body(c, fn_node);
//             //     ClosureObj *closure = alloc<ClosureObj>(c.vm, fn, 0);
//             //     vm.globals[c.idarr[c.fn_node->id].idx] = MK_OBJ(closure);
//             //     if (fn_node.span == "main")
//             //         main = closure;
//             // } else {
//             //     const auto &class_node = static_cast<const ClassDeclNode &>(*node.decls[i]);
//             //     ClassObj *klass = alloc<ClassObj>(c.vm, alloc<StringObj>(c.vm, class_node.span));
//             //     for (i32 i = 0; i < class_node.cnt; i++) {
//             //         const auto &fn_node = static_cast<const FnDeclNode &>(*class_node.methods[i]);
//             //         FnObj *fn = alloc<FnObj>(c.vm, alloc<StringObj>(c.vm, fn_node.span), Chunk(), fn_node.arity);
//             //         c.fn_node = &fn_node;
//             //         c.fn = fn;
//             //         compile_fn_body(c, fn_node);
//             //         ClosureObj *closure = alloc<ClosureObj>(c.vm, fn, 0);
//             //         klass->methods.insert(*alloc<StringObj>(c.vm, fn_node.span), MK_OBJ(closure));
//             //     }
//             //     vm.globals[c.idarr[class_node.id].idx] = MK_OBJ(klass);
//             // }
//         }
//         return main;
//     }
// };

// ClosureObj *compile(VM &vm, ModuleNode &node, Dynarr<ErrMsg> &errarr)
// {
//     Compiler compiler(vm, errarr);
//     return compiler.visit(node);
// }