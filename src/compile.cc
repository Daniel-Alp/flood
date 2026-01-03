// #include <assert.h>
// #include <string.h>
// #include <stdlib.h>
// #include "compile.h"
// #include "symbol.h"
// #include "parse.h"

// static struct Chunk *cur_chunk(struct Compiler *compiler) 
// {
//     return &compiler->fn->chunk;
// }

// static struct Symbol *symbols(struct Compiler *compiler)
// {
//     return compiler->sym_arr->symbols;
// }

// static i32 emit_jump(struct Compiler *compiler, enum OpCode op, i32 line) 
// {
//     i32 offset = cur_chunk(compiler)->cnt;
//     emit_byte(cur_chunk(compiler), op, line);
//     // skip two bytes for jump
//     //      OP_JUMP
//     //      hi
//     //      lo
//     emit_byte(cur_chunk(compiler), 0, line);
//     emit_byte(cur_chunk(compiler), 0, line);
//     return offset;
// }

// static void patch_jump(struct Compiler *compiler, struct Span span, i32 offset) 
// {
//     // offset idx of the OP_JUMP instr
//     // cnt is idx of to-be-executed instr
//     i32 jump = cur_chunk(compiler)->cnt - (offset+3);
//     if (jump > ((1 << 16) - 1))
//         push_errarr(&compiler->errarr, span, "jump too far");
//     cur_chunk(compiler)->code[offset+1] = (jump >> 8) & 0xff;
//     cur_chunk(compiler)->code[offset+2] = jump & 0xff; 
// }

// static void emit_constant(struct Compiler *compiler, Value val, i32 line)
// {
//     emit_byte(cur_chunk(compiler), OP_GET_CONST, line);
//     emit_byte(cur_chunk(compiler), add_constant(cur_chunk(compiler), val), line);
// }

// static void compile_node(struct Compiler *compiler, struct Node *node);

// static void compile_atom(struct Compiler *compiler, struct AtomNode *node) 
// {
//     i32 line = node->base.span.line;
//     switch(node->atom_tag) {
//     case TOKEN_NULL:   emit_byte(cur_chunk(compiler), OP_NULL, line); break;
//     case TOKEN_TRUE:   emit_byte(cur_chunk(compiler), OP_TRUE, line); break;
//     case TOKEN_FALSE:  emit_byte(cur_chunk(compiler), OP_FALSE, line); break;
//     case TOKEN_NUMBER: emit_constant(compiler, MK_NUM(strtod(node->base.span.start, NULL)), line); break;
//     case TOKEN_STRING: emit_constant(compiler, MK_OBJ((struct Obj*)string_from_span(compiler->vm, node->base.span)), line); break; 
//     default:           push_errarr(&compiler->errarr, node->base.span, "default case of compile_atom reached");
//     }
// }

// // given an identifier that is captured return what it's idx will be 
// // in the fn's capture array, or -1 if this fn does not capture it
// static i32 resolve_capture(struct FnDeclNode *node, i32 id)
// {
//     // capture count cannot exceed MAX_LOCALS
//     for (i32 j = 0; j < MAX_LOCALS; j++) {
//         if (j < node->stack_capture_cnt && id == node->stack_captures[j])
//             return j;
//         if (j < node->parent_capture_cnt && id == node->parent_captures[j])
//             return j + node->stack_capture_cnt;
//     }
//     return -1;
// }

// static void compile_ident_get_or_set(struct Compiler *compiler, i32 id, bool get, i32 line)
// {
//     struct Symbol sym = symbols(compiler)[id];
//     // special case if a function is referencing itself we can do OP_GET_LOCAL 0 or OP_SET_LOCAL 0
//     if (id == compiler->fn_node->id) {
//         emit_byte(cur_chunk(compiler), get ? OP_GET_LOCAL : OP_SET_LOCAL, line);
//         emit_byte(cur_chunk(compiler), 0, line);
//         return;
//     }
//     if (sym.depth == 0) {
//         emit_byte(cur_chunk(compiler), get ? OP_GET_GLOBAL : OP_SET_GLOBAL, line);
//         emit_byte(cur_chunk(compiler), sym.idx, line);
//         return;
//     }
//     if (sym.flags & FLAG_CAPTURED) {
//         i32 captures_arr_idx = resolve_capture(compiler->fn_node, id);
//         // ptr to the value lives on the stack
//         if (captures_arr_idx == -1) {
//             emit_byte(cur_chunk(compiler), get ? OP_GET_HEAPVAL : OP_SET_HEAPVAL, line);
//             emit_byte(cur_chunk(compiler), sym.idx, line);
//             return;
//         }
//         emit_byte(cur_chunk(compiler), get ? OP_GET_CAPTURED : OP_SET_CAPTURED, line);
//         emit_byte(cur_chunk(compiler), captures_arr_idx, line);
//         return;
//     } 
//     emit_byte(cur_chunk(compiler), get ? OP_GET_LOCAL : OP_SET_LOCAL, line);
//     emit_byte(cur_chunk(compiler), sym.idx, line);
// }

// // ident get
// static void compile_ident(struct Compiler *compiler, struct IdentNode *node) 
// {
//     compile_ident_get_or_set(compiler, node->id, true, node->base.span.line);
// }

// static void compile_list(struct Compiler *compiler, struct ListNode *node) 
// {
//     // TODO allow lists to be initialized with more than 256 elements
//     // TODO handle lists being initialized with too many elements
//     i32 line = node->base.span.line;
//     for (i32 i = 0; i < node->cnt; i++)
//         compile_node(compiler, node->items[i]);    
//     emit_byte(cur_chunk(compiler), OP_LIST, line);
//     emit_byte(cur_chunk(compiler), node->cnt, line);
// }

// static void compile_unary(struct Compiler *compiler, struct UnaryNode *node) 
// {
//     i32 line = node->base.span.line;
//     compile_node(compiler, node->rhs);
//     if (node->op_tag == TOKEN_MINUS)
//         emit_byte(cur_chunk(compiler), OP_NEGATE, line);
//     else if (node->op_tag == TOKEN_NOT)
//         emit_byte(cur_chunk(compiler), OP_NOT, line);
// }

// static void compile_binary(struct Compiler *compiler, struct BinaryNode *node) 
// {
//     i32 line = node->base.span.line;
//     enum TokenTag op_tag = node->op_tag;
//     if (op_tag == TOKEN_EQ) {
//         if (node->lhs->tag == NODE_IDENT) {
//             compile_node(compiler, node->rhs);
//             // ident set
//             compile_ident_get_or_set(compiler, ((struct IdentNode*)node->lhs)->id, false, node->lhs->span.line);
//         } else if (node->lhs->tag == NODE_BINARY && ((struct BinaryNode*)node->lhs)->op_tag == TOKEN_L_SQUARE) {
//             // list set
//             compile_node(compiler, ((struct BinaryNode*)node->lhs)->lhs);
//             compile_node(compiler, ((struct BinaryNode*)node->lhs)->rhs);
//             compile_node(compiler, node->rhs);
//             emit_byte(cur_chunk(compiler), OP_SET_SUBSCR, line);
//         } else if (node->lhs->tag == NODE_PROPERTY && ((struct PropertyNode*)node->lhs)->op_tag == TOKEN_DOT) {
//             // field set
//             compile_node(compiler, ((struct PropertyNode*)node->lhs)->lhs);
//             compile_node(compiler, node->rhs);
//             emit_byte(cur_chunk(compiler), OP_SET_FIELD, line);
//             struct StringObj *str = string_from_span(compiler->vm, ((struct PropertyNode*)node->lhs)->sym);
//             emit_byte(cur_chunk(compiler), add_constant(cur_chunk(compiler), MK_OBJ((struct Obj*)str)), line);
//         }
//     } else if (op_tag == TOKEN_AND || op_tag == TOKEN_OR) {
//         compile_node(compiler, node->lhs);
//         i32 offset = emit_jump(compiler, op_tag == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE, line);
//         emit_byte(cur_chunk(compiler), OP_POP, line);
//         compile_node(compiler, node->rhs);
//         patch_jump(compiler, node->base.span, offset);
//     } else {
//         compile_node(compiler, node->lhs);
//         compile_node(compiler, node->rhs);
//         switch (op_tag) {
//         case TOKEN_PLUS:        emit_byte(cur_chunk(compiler), OP_ADD, line); break;
//         case TOKEN_MINUS:       emit_byte(cur_chunk(compiler), OP_SUB, line); break;
//         case TOKEN_STAR:        emit_byte(cur_chunk(compiler), OP_MUL, line); break;
//         case TOKEN_SLASH:       emit_byte(cur_chunk(compiler), OP_DIV, line); break;
//         case TOKEN_SLASH_SLASH: emit_byte(cur_chunk(compiler), OP_FLOORDIV, line); break;
//         case TOKEN_PERCENT:     emit_byte(cur_chunk(compiler), OP_MOD, line); break;
//         case TOKEN_LT:          emit_byte(cur_chunk(compiler), OP_LT, line); break; 
//         case TOKEN_LEQ:         emit_byte(cur_chunk(compiler), OP_LEQ, line); break;
//         case TOKEN_GT:          emit_byte(cur_chunk(compiler), OP_GT, line); break;
//         case TOKEN_GEQ:         emit_byte(cur_chunk(compiler), OP_GEQ, line); break;
//         case TOKEN_EQEQ:        emit_byte(cur_chunk(compiler), OP_EQEQ, line); break;
//         case TOKEN_NEQ:         emit_byte(cur_chunk(compiler), OP_NEQ, line); break;
//         case TOKEN_L_SQUARE:    emit_byte(cur_chunk(compiler), OP_GET_SUBSCR, line); break;
//         default:                push_errarr(&compiler->errarr, node->base.span, "default case of compile_binary reached");
//         }
//     }
// }

// static void compile_get_property(struct Compiler *compiler, struct PropertyNode *node)
// {
//     i32 line = node->base.span.line;
//     compile_node(compiler, node->lhs);
//     struct StringObj *str = string_from_span(compiler->vm, node->sym);
//     emit_byte(cur_chunk(compiler), node->op_tag == TOKEN_DOT ? OP_GET_FIELD : OP_GET_METHOD, line);    
//     emit_byte(cur_chunk(compiler), add_constant(cur_chunk(compiler), MK_OBJ((struct Obj*)str)), line);
// }

// static void compile_call(struct Compiler *compiler, struct CallNode *node) 
// {
//     i32 line = node->base.span.line;
//     compile_node(compiler, node->lhs);
//     for (i32 i = 0; i < node->arity; i++)
//         compile_node(compiler, node->args[i]);
//     emit_byte(cur_chunk(compiler), OP_CALL, line);
//     emit_byte(cur_chunk(compiler), node->arity, line);
//     // since there may be any number of locals on the stack when the callee returns
//     // the callee is responsible for cleanup and moving the return value
// }

// static void compile_block(struct Compiler *compiler, struct BlockNode *node) 
// {
//     i32 line = node->base.span.line;
//     i32 local_cnt = compiler->fn_local_cnt;
//     for (i32 i = 0; i < node->cnt; i++)
//         compile_node(compiler, node->stmts[i]);
//     // TODO handle more than 256 locals 
//     i32 n = compiler->fn_local_cnt - local_cnt;
//     if (n == 1) {
//         emit_byte(cur_chunk(compiler), OP_POP, line);
//     } else if (n > 1) {
//         emit_byte(cur_chunk(compiler), OP_POP_N, line);
//         emit_byte(cur_chunk(compiler), n, line);
//     }
//     compiler->fn_local_cnt = local_cnt;
// }

// static void compile_if(struct Compiler *compiler, struct IfNode *node) 
// {
//     i32 line = node->base.span.line;
//     compile_node(compiler, node->cond);
//     //      OP_JUMP_IF_FALSE (jump 1)
//     //      OP_POP           
//     //      thn block              
//     //      OP_JUMP          (jump 2)
//     //      OP_POP           (destination of jump 1)
//     //      els block        
//     //      ...              (destination of jump 2)
//     i32 offset1 = emit_jump(compiler, OP_JUMP_IF_FALSE, line);
//     emit_byte(cur_chunk(compiler), OP_POP, line);

//     compile_block(compiler, node->thn);
//     i32 offset2 = emit_jump(compiler, OP_JUMP, line);
//     patch_jump(compiler, node->base.span, offset1);
    
//     emit_byte(cur_chunk(compiler), OP_POP, line);
//     if (node->els)
//         compile_block(compiler, node->els);
//     patch_jump(compiler, node->base.span, offset2);
// }

// static void compile_expr_stmt(struct Compiler *compiler, struct ExprStmtNode *node) 
// {
//     compile_node(compiler, node->expr);
//     emit_byte(cur_chunk(compiler), OP_POP, node->base.span.line);
// }

// static void compile_return(struct Compiler *compiler, struct ReturnNode *node) 
// {
//     i32 line = node->base.span.line;
//     if (node->expr) {
//         compile_node(compiler, node->expr);
//     } else if (symbols(compiler)[compiler->fn_node->id].flags & FLAG_INIT) {
//         // `init` method implicitly returns `self` which is at bp[arity]
//         emit_byte(cur_chunk(compiler), OP_GET_LOCAL, line);
//         emit_byte(cur_chunk(compiler), compiler->fn_node->arity, line);
//     } else {
//         emit_byte(cur_chunk(compiler), OP_NULL, line);
//     }
//     emit_byte(cur_chunk(compiler), OP_RETURN, line);
// }

// static void compile_print(struct Compiler *compiler, struct PrintNode *node) 
// {
//     compile_node(compiler, node->expr);
//     emit_byte(cur_chunk(compiler), OP_PRINT, node->base.span.line);
// }

// static void compile_var_decl(struct Compiler *compiler, struct VarDeclNode *node) 
// {
//     i32 line = node->base.span.line;
//     // TODO var decls for class members 
//     struct Symbol* sym = &symbols(compiler)[node->id];
//     sym->idx = compiler->fn_local_cnt;
//     compiler->fn_local_cnt++;
//     if (node->init)
//         compile_node(compiler, node->init);
//     else 
//         emit_byte(cur_chunk(compiler), OP_NULL, line);
//     // move variable on heap if it is captured
//     if (sym->flags & FLAG_CAPTURED) {
//         emit_byte(cur_chunk(compiler), OP_HEAPVAL, line);
//         emit_byte(cur_chunk(compiler), sym->idx, line);
//     }
// }

// static void compile_fn_decl(struct Compiler *compiler, struct FnDeclNode *node)
// {
//     i32 line = node->base.span.line;
//     struct Symbol* sym = &symbols(compiler)[node->id];
//     // top level functions are hoisted
//     if (sym->idx == -1) {
//         sym->idx = compiler->fn_local_cnt;
//         compiler->fn_local_cnt++;
//     }

//     struct FnObj* fn = (struct FnObj*)alloc_vm_obj(compiler->vm, sizeof(struct FnObj));
//     init_fn_obj(fn, string_from_span(compiler->vm, node->base.span), node->arity);

//     // push state and enter the fn
//     i32 parent_local_cnt = compiler->fn_local_cnt;
//     struct FnObj* parent = compiler->fn; 
//     struct FnDeclNode *parent_node = compiler->fn_node;
//     compiler->fn_local_cnt = 1;
//     compiler->fn = fn;
//     compiler->fn_node = node;
//     for (i32 i = 0; i < node->arity; i++) {
//         struct Symbol *param_sym = &symbols(compiler)[node->params[i].id];
//         param_sym->idx = compiler->fn_local_cnt;
//         compiler->fn_local_cnt++;
//         // move param on heap if it is captured
//         if (param_sym->flags & FLAG_CAPTURED) {
//             emit_byte(cur_chunk(compiler), OP_HEAPVAL, line);
//             emit_byte(cur_chunk(compiler), param_sym->idx, line);
//         }
//     }
//     compile_block(compiler, node->body);
//     if (sym->flags & FLAG_INIT) {
//         // `init` method implicitly returns `self` which is at bp[arity]
//         emit_byte(cur_chunk(compiler), OP_GET_LOCAL, node->base.span.line);
//         emit_byte(cur_chunk(compiler), compiler->fn_node->arity, node->base.span.line);
//     } else {
//         emit_byte(cur_chunk(compiler), OP_NULL, line);
//     }
//     emit_byte(cur_chunk(compiler), OP_RETURN, line);
//     // disassemble_chunk(cur_chunk(compiler), compiler->fn->name->chars);

//     // pop state and exit the fn
//     compiler->fn_local_cnt = parent_local_cnt;
//     compiler->fn = parent;
//     compiler->fn_node = parent_node;
//     emit_constant(compiler, MK_OBJ((struct Obj*)fn), line);
//     // wrap the fn in a closure
//     emit_byte(cur_chunk(compiler), OP_CLOSURE, line);
//     emit_byte(cur_chunk(compiler), node->stack_capture_cnt, line);
//     emit_byte(cur_chunk(compiler), node->parent_capture_cnt, line);
//     for (i32 i = 0; i < node->stack_capture_cnt; i++) {
//         i32 id = node->stack_captures[i];
//         emit_byte(cur_chunk(compiler), symbols(compiler)[id].idx, line);
//     }
//     for (i32 i = 0; i < node->parent_capture_cnt; i++) {
//         i32 id = node->parent_captures[i];
//         // node->parent can't be NULL
//         emit_byte(cur_chunk(compiler), resolve_capture(node->parent, id), line);
//     }
//     // move closure on heap if it is captured
//     if (sym->flags & FLAG_CAPTURED) {
//         emit_byte(cur_chunk(compiler), OP_HEAPVAL, line);
//         emit_byte(cur_chunk(compiler), sym->idx, line);
//     }
// }

// static void compile_class_decl(struct Compiler *compiler, struct ClassDeclNode *node)
// {
//     i32 line = node->base.span.line;
//     struct Symbol* sym = &symbols(compiler)[node->id];
//     // top level functions are hoisted
//     if (sym->idx == -1) {
//         sym->idx = compiler->fn_local_cnt;
//         compiler->fn_local_cnt++;
//     }

//     emit_byte(cur_chunk(compiler), OP_CLASS, line);
//     struct StringObj *str = string_from_span(compiler->vm, node->base.span);
//     emit_byte(cur_chunk(compiler), add_constant(cur_chunk(compiler), MK_OBJ((struct Obj*)str)), line);

//     for (i32 i = 0; i < node->cnt; i++) {
//         compile_fn_decl(compiler, node->methods[i]);
//         emit_byte(cur_chunk(compiler), OP_METHOD, line);
//     }
// }

// static void compile_node(struct Compiler *compiler, struct Node *node) 
// {
//     switch (node->tag) {
//     case NODE_ATOM:       compile_atom(compiler, (struct AtomNode*)node); break;
//     case NODE_LIST:       compile_list(compiler, (struct ListNode*)node); break;
//     case NODE_IDENT:      compile_ident(compiler, (struct IdentNode*)node); break;
//     case NODE_UNARY:      compile_unary(compiler, (struct UnaryNode*)node); break;
//     case NODE_BINARY:     compile_binary(compiler, (struct BinaryNode*)node); break;
//     case NODE_PROPERTY:   compile_get_property(compiler, (struct PropertyNode*)node); break;
//     case NODE_CALL:       compile_call(compiler, (struct CallNode*)node); break;
//     case NODE_VAR_DECL:   compile_var_decl(compiler, (struct VarDeclNode*)node); break;
//     case NODE_FN_DECL:    compile_fn_decl(compiler, (struct FnDeclNode*)node); break;
//     case NODE_CLASS_DECL: compile_class_decl(compiler, (struct ClassDeclNode*)node); break;
//     case NODE_IMPORT:     push_errarr(&compiler->errarr, node->span, "TODO"); break;
//     case NODE_EXPR_STMT:  compile_expr_stmt(compiler, (struct ExprStmtNode*)node); break; 
//     case NODE_BLOCK:      compile_block(compiler, (struct BlockNode*)node); break;
//     case NODE_IF:         compile_if(compiler, (struct IfNode*)node); break;
//     case NODE_RETURN:     compile_return(compiler, (struct ReturnNode*)node); break;
//     // TEMP remove when we add functions
//     case NODE_PRINT:      compile_print(compiler, (struct PrintNode*)node); break; 
//     }
// }

// // TODO this isn't important right now but it's weird 
// // that the compiler takes some arguments such as sym arr upon initialization 
// // and others like VM each time you call compile_file
// // later I'll probably have it so that the compiler owns its errlist but everything else 
// // is passed in as args. This way you can use a single compiler struct for however many VMs you like.
// void init_compiler(struct Compiler *compiler, struct SymArr *arr) 
// {
//     init_errarr(&compiler->errarr);
//     compiler->main_fn_idx = -1;
//     compiler->sym_arr = arr;
//     compiler->vm = NULL;
// }

// void release_compiler(struct Compiler *compiler)
// {
//     release_errarr(&compiler->errarr);
//     compiler->main_fn_idx = -1;
//     // compiler does not own these
//     compiler->sym_arr = NULL;
//     compiler->vm = NULL;
// }

// struct ClosureObj *compile_file(struct VM *vm, struct Compiler *compiler, struct FnDeclNode *node) 
// {    
//     // TODO I should probably be clearing the errorlist each time
//     // have to clear other things in the compiler as well each time we compile
//     compiler->vm = vm;

//     struct FnObj *top_fn = (struct FnObj*)alloc_vm_obj(vm, sizeof(struct FnObj));
//     init_fn_obj(top_fn, string_from_c_str(vm, "script"), 0);

//     struct ClosureObj *top_closure = (struct ClosureObj*)alloc_vm_obj(vm, sizeof(struct ClosureObj));
//     init_closure_obj(top_closure, top_fn, 0);

//     compiler->fn_node = node;
//     compiler->fn = top_fn;
//     compiler->fn_local_cnt = 0;

//     for (i32 i = 0; i < node->body->cnt; i++) {
//         // for top-level functions and classes we set their index before we compile them, hosting them up
//         if (node->body->stmts[i]->tag == NODE_FN_DECL) {
//             struct FnDeclNode *fn_node = (struct FnDeclNode*)node->body->stmts[i];
//             symbols(compiler)[fn_node->id].idx = compiler->fn_local_cnt;
//             struct Span fn_span = fn_node->base.span;
//             if (fn_span.len == 4 && strncmp(fn_span.start, "main", 4) == 0)
//                 compiler->main_fn_idx = compiler->fn_local_cnt;
//         } else {
//             struct ClassDeclNode *class_node = (struct ClassDeclNode*)node->body->stmts[i];
//             symbols(compiler)[class_node->id].idx = compiler->fn_local_cnt;   
//         }
//         compiler->fn_local_cnt++;
//     }
//     for (i32 i = 0; i < node->body->cnt; i++) {
//         struct Node *stmt = node->body->stmts[i];
//         if (stmt->tag == NODE_FN_DECL)
//             compile_fn_decl(compiler, (struct FnDeclNode*)stmt);
//         else
//             compile_class_decl(compiler, (struct ClassDeclNode*)stmt);
//         i32 line = stmt->span.line;
//         emit_byte(cur_chunk(compiler), OP_SET_GLOBAL, line);
//         emit_byte(cur_chunk(compiler), i, line);
//         emit_byte(cur_chunk(compiler), OP_POP, line);
//     }

//     // disassemble_chunk(cur_chunk(compiler), compiler->fn->name->chars);

//     return top_closure;
// }
