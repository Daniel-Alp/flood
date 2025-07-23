#include <string.h>
#include <stdlib.h>
#include "sema.h"

// TEMP remove globals when we added user-defined classes

void init_sema_state(struct SemaState *sema, struct SymArr *sym_arr) 
{
    sema->depth = 0;
    sema->local_cnt = 0;
    sema->global_cnt = 0;
    sema->sym_arr = sym_arr;
    sema->fn = NULL;
    init_errlist(&sema->errlist);
}

void release_sema_state(struct SemaState *sema)
{
    sema->local_cnt = 0;
    sema->global_cnt = 0;
    sema->depth = 0;
    // sema does not own sym_arr
    sema->sym_arr = NULL;
    release_errlist(&sema->errlist);
}

// return id of ident or -1
static i32 resolve_ident(struct SemaState *sema, struct Span span)
{
    for (i32 i = sema->local_cnt-1; i >= 0; i--) {
        struct Span other = sema->sym_arr->symbols[sema->locals[i]].span;        
        if (span.len == other.len && memcmp(span.start, other.start, span.len) == 0) 
            return sema->locals[i];
    }
    for (i32 i = sema->global_cnt-1; i >= 0; i--) {
        struct Span other = sema->sym_arr->symbols[sema->globals[i]].span;        
        if (span.len == other.len && memcmp(span.start, other.start, span.len) == 0) 
            return sema->globals[i];
    }
    return -1;
}

static i32 declare_ident(struct SemaState *sema, struct Span span)
{
    // TODO we can make this faster by only checking in the current scope
    i32 id = resolve_ident(sema, span);
    if (id != -1 && sema->sym_arr->symbols[id].depth == sema->depth)
        push_errlist(&sema->errlist, span, "redeclared variable");
    struct Symbol sym = {
        .span  = span,
        .flags = FLAG_NONE,
        .depth = sema->depth,
        .idx   = -1
    };
    id = push_symbol_arr(sema->sym_arr, sym);
    // TODO error if more than 256 locals or 256 globals
    if (sema->depth > 0) {
        sema->locals[sema->local_cnt] = id;
        sema->local_cnt++;
    } else {
        sema->globals[sema->global_cnt] = id;
        sema->global_cnt++;
    }
    return id;
}

// NOTE: 
// given a fn and an ident, we update the capture arrs of the fn
// we do this
//      (1) when we analyze an ident we update the stack_captures and parent_captures of the fn
//      (2) after analyzing fn body we propagate captures to its parent. for example
//          fn foo() {
//              var x = 1;
//              fn bar() {
//                  var y = 2;
//                  fn baz() {
//                      print x + y;
//                  }
//                  return baz;
//              }
//              return bar;
//          }
//          in this case y is in the stack_captures of baz and x is in the parent_captures of baz 
//          we take every ident in the parent_captures of baz, and use it to update the captures arrs of bar
static void update_captures(struct SemaState *sema, i32 id)
{
    struct Symbol *sym = &sema->sym_arr->symbols[id];
    struct FnDeclNode *fn = sema->fn;
    struct FnDeclNode *parent = fn->parent;
    // NOTE: 
    // special cases. 
    // if sym->depth == 0 the ident is global so we don't need to capture it.
    // if id == fn->id then the fn we're compiling needs a ptr to itself,
    // we can get this from the current stack frame.
    if (sym->depth > sema->sym_arr->symbols[fn->id].depth || sym->depth == 0 || id == fn->id)
        return;
    for (i32 i = 0; i < fn->stack_capture_cnt; i++) {
        if (id == fn->stack_captures[i])
            return;
    }
    for (i32 i = 0; i < fn->parent_capture_cnt; i++) {
        if (id == fn->parent_captures[i])
            return;
    } 
    sym->flags |= FLAG_CAPTURED; 
    // TODO handle more than 256 captures
    if (!parent || sym->depth > sema->sym_arr->symbols[parent->id].depth || id == parent->id) {
        fn->stack_captures[fn->stack_capture_cnt] = id;
        fn->stack_capture_cnt++;
    } else {
        fn->parent_captures[fn->parent_capture_cnt] = id;
        fn->parent_capture_cnt++;
    }
}

static void analyze_node(struct SemaState *sema, struct Node *node);

static void analyze_ident(struct SemaState *sema, struct IdentNode *node)
{
    i32 id = resolve_ident(sema, node->base.span);
    if (id == -1) {
        push_errlist(&sema->errlist, node->base.span, "not found in this scope");
        return;
    }
    node->id = id;
    update_captures(sema, id);
}

static void analyze_list(struct SemaState *sema, struct ListNode *node)
{
    for (i32 i = 0; i < node->cnt; i++)
        analyze_node(sema, node->items[i]);
}

static void analyze_unary(struct SemaState *sema, struct UnaryNode *node)
{
    analyze_node(sema, node->rhs);
}

static void analyze_binary(struct SemaState *sema, struct BinaryNode *node) 
{
    if (node->op_tag == TOKEN_EQ) {
        bool ident = node->lhs->tag == NODE_IDENT;
        bool list_elem = node->lhs->tag == NODE_BINARY && ((struct BinaryNode*)node->lhs)->op_tag == TOKEN_L_SQUARE;
        if (!ident && !list_elem)
            push_errlist(&sema->errlist, node->base.span, "cannot assign to left-hand expression");
    }
    analyze_node(sema, node->lhs);
    analyze_node(sema, node->rhs);
}

static void analyze_get_prop(struct SemaState *sema, struct PropNode *node)
{
    analyze_node(sema, node->lhs);
}

static void analyze_fn_call(struct SemaState *sema, struct CallNode *node) 
{
    analyze_node(sema, node->lhs);
    for (i32 i = 0; i < node->arity; i++)
        analyze_node(sema, node->args[i]);
}

static void analyze_block(struct SemaState *sema, struct BlockNode *node) 
{
    i32 local_cnt = sema->local_cnt;
    sema->depth++;
    for (i32 i = 0; i < node->cnt; i++)
        analyze_node(sema, node->stmts[i]);
    sema->depth--;
    sema->local_cnt = local_cnt;
}

static void analyze_if(struct SemaState *sema, struct IfNode *node) 
{
    analyze_node(sema, node->cond);
    analyze_block(sema, node->thn);
    if (node->els)
        analyze_block(sema, node->els);
}

static void analyze_expr_stmt(struct SemaState *sema, struct ExprStmtNode *node) 
{
    analyze_node(sema, node->expr);
}

static void analyze_print(struct SemaState *sema, struct PrintNode *node) 
{
    analyze_node(sema, node->expr);
}

static void analyze_return(struct SemaState *sema, struct ReturnNode *node) 
{
    if (node->expr)
        analyze_node(sema, node->expr);
}

static void analyze_var_decl(struct SemaState *sema, struct VarDeclNode *node) 
{
    node->id = declare_ident(sema, node->base.span);
    if (node->init)
        analyze_node(sema, node->init);
}

static void analyze_fn_body(struct SemaState *sema, struct FnDeclNode *node)
{
    // push state and enter the fn
    node->parent = sema->fn;
    sema->fn = node;
    i32 local_cnt = sema->local_cnt;

    sema->depth++;
    for (i32 i = 0; i < node->arity; i++)
        node->params[i].id = declare_ident(sema, node->params[i].base.span);
    sema->depth--;
    analyze_block(sema, node->body);

    // pop state and leave the fn
    sema->local_cnt = local_cnt;
    sema->fn = node->parent;
    
    for (i32 i = 0; i < node->parent_capture_cnt; i++)
        update_captures(sema, node->parent_captures[i]);
}

static void analyze_class_body(struct SemaState *sema, struct ClassDeclNode *node)
{
    for (i32 i = 0; i < node->cnt; i++) {
        struct FnDeclNode *fn_decl = (struct FnDeclNode*)node->methods[i];
        fn_decl->id = declare_ident(sema, fn_decl->base.span);           
        analyze_fn_body(sema, node->methods[i]);
    }
}

static void analyze_node(struct SemaState *sema, struct Node *node)
{
    switch (node->tag) {
    case NODE_ATOM:      return;
    case NODE_LIST:      analyze_list(sema, (struct ListNode*)node); break;
    case NODE_IDENT:     analyze_ident(sema, (struct IdentNode*)node); break;
    case NODE_UNARY:     analyze_unary(sema, (struct UnaryNode*)node); break;
    case NODE_BINARY:    analyze_binary(sema, (struct BinaryNode*)node); break;
    case NODE_PROP:      analyze_get_prop(sema, (struct PropNode*)node); break;
    case NODE_CALL:      analyze_fn_call(sema, (struct CallNode*)node); break;
    case NODE_BLOCK:     analyze_block(sema, (struct BlockNode*)node); break;
    case NODE_IF:        analyze_if(sema, (struct IfNode*)node); break;
    case NODE_EXPR_STMT: analyze_expr_stmt(sema, (struct ExprStmtNode*)node); break;
    case NODE_RETURN:    analyze_return(sema, (struct ReturnNode*)node); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     analyze_print(sema, (struct PrintNode*)node); break;
    case NODE_VAR_DECL:  analyze_var_decl(sema, (struct VarDeclNode*)node); break;
    case NODE_FN_DECL: {
        struct FnDeclNode *fn_decl = (struct FnDeclNode*)node;
        fn_decl->id = declare_ident(sema, fn_decl->base.span);        
        analyze_fn_body(sema, fn_decl);
        break;
    }
    case NODE_CLASS_DECL: {
        struct ClassDeclNode *class_decl = (struct ClassDeclNode*)node;
        class_decl->id = declare_ident(sema, class_decl->base.span);        
        analyze_class_body(sema, class_decl);
    }
    }
}

void analyze(struct SemaState *sema, struct FnDeclNode *node)
{
    // TODO I should probably be clearing the errorlist each time
    struct BlockNode *body = node->body;
    for (i32 i = 0; i < body->cnt; i++) {
        if (body->stmts[i]->tag == NODE_FN_DECL) {
            struct FnDeclNode *fn_decl = (struct FnDeclNode*)body->stmts[i];
            fn_decl->id = declare_ident(sema, fn_decl->base.span);
        } else {
            struct ClassDeclNode *class_decl = (struct ClassDeclNode*)body->stmts[i];
            class_decl->id = declare_ident(sema, class_decl->base.span);
        }
    } 
    for (i32 i = 0; i < body->cnt; i++) {
        if (body->stmts[i]->tag == NODE_FN_DECL) 
            analyze_fn_body(sema, (struct FnDeclNode*)body->stmts[i]);
        else
            analyze_class_body(sema, (struct ClassDeclNode*)body->stmts[i]);
    }
}