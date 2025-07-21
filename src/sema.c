#include <string.h>
#include <stdlib.h>
#include "sema.h"

// TEMP remove globals when we added user-defined classes

void init_sema_state(struct SemaState *sema, struct SymArr *sym_arr) 
{
    sema->depth = 0;
    sema->local_cnt = 0;
    sema->sym_arr = sym_arr;
    sema->fn = NULL;
    init_errlist(&sema->errlist);
}

void release_sema_state(struct SemaState *sema)
{
    sema->local_cnt = 0;
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
    return -1;
}

static void analyze_node(struct SemaState *sema, struct Node *node);

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

static void analyze_fn_call(struct SemaState *sema, struct FnCallNode *node) 
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
    struct Span span = node->base.span;
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
    node->id = id;
    if (node->init)
        analyze_node(sema, node->init);
    // TODO error if more than 256 locals
    // NOT:
    // I don't support global variables because globals are being replaced by a different system altogether
    sema->locals[sema->local_cnt] = id;
    sema->local_cnt++;
}

// NOTE: 
// we split analyzing the function signature and body 
// to allow for mutually recursive functions at the top level
static void analyze_fn_signature(struct SemaState *sema, struct FnDeclNode *node) 
{
    struct Span fn_span = node->base.span;
    i32 id = resolve_ident(sema, fn_span);
    if (id != -1 && sema->sym_arr->symbols[id].depth == sema->depth)
        push_errlist(&sema->errlist, fn_span, "redeclared variable");
    struct Symbol sym = {
        .span  = fn_span,
        .flags = FLAG_NONE,
        .depth = sema->depth,
        .idx   = -1
    };
    id = push_symbol_arr(sema->sym_arr, sym);
    node->id = id;
    // TODO error if more than 256 locals 
    sema->locals[sema->local_cnt] = id;
    sema->local_cnt++;
    for (i32 i = 0; i < node->arity; i++) {
        struct Span param_span = node->params[i].base.span;
        for (i32 j = 0; j < i; j++) {
            struct Span other = node->params[j].base.span;
            if (other.len == param_span.len && memcmp(other.start, param_span.start, param_span.len) == 0) {
                push_errlist(&sema->errlist, param_span, "used as parameter more than once");
                break;
            }
        }
    }
}

static void analyze_fn_body(struct SemaState *sema, struct FnDeclNode *node)
{
    // push state and enter the fn
    node->parent = sema->fn;
    sema->fn = node;
    i32 local_cnt = sema->local_cnt;

    for (i32 i = 0; i < node->arity; i++) {
        struct Span span = node->params[i].base.span;
        struct Symbol sym = {
            .span = span,
            .flags = FLAG_NONE,
            .depth = sema->depth + 1,
            .idx   = -1
        };
        i32 id = push_symbol_arr(sema->sym_arr, sym);
        node->params[i].id = id;
        // TODO error if more than 256 locals
        sema->locals[sema->local_cnt] = id;
        sema->local_cnt++;
    }
    analyze_block(sema, node->body);

    // pop state and leave the fn
    sema->local_cnt = local_cnt;
    sema->fn = node->parent;
    
    for (i32 i = 0; i < node->parent_capture_cnt; i++)
        update_captures(sema, node->parent_captures[i]);
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
    case NODE_FN_CALL:   analyze_fn_call(sema, (struct FnCallNode*)node); break;
    case NODE_BLOCK:     analyze_block(sema, (struct BlockNode*)node); break;
    case NODE_IF:        analyze_if(sema, (struct IfNode*)node); break;
    case NODE_EXPR_STMT: analyze_expr_stmt(sema, (struct ExprStmtNode*)node); break;
    case NODE_RETURN:    analyze_return(sema, (struct ReturnNode*)node); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     analyze_print(sema, (struct PrintNode*)node); break;
    case NODE_VAR_DECL:  analyze_var_decl(sema, (struct VarDeclNode*)node); break;
    case NODE_FN_DECL: {
        analyze_fn_signature(sema, (struct FnDeclNode*)node);
        analyze_fn_body(sema, (struct FnDeclNode*)node);
        break;
    }
    }
}

void analyze(struct SemaState *sema, struct FnDeclNode *node)
{
    // TODO I should probably be clearing the errorlist each time
    struct BlockNode *body = node->body;
    for (i32 i = 0; i < body->cnt; i++) 
        analyze_fn_signature(sema, (struct FnDeclNode*)body->stmts[i]);
    for (i32 i = 0; i < body->cnt; i++)
        analyze_fn_body(sema, (struct FnDeclNode*)body->stmts[i]);
}