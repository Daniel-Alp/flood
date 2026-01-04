#include <stdlib.h>
#include "sema.h"
#include "error.h"
#include "parse.h"
#include "scan.h"

// return id of ident or -1
static i32 resolve_ident(const SemaCtx &s, const Span span)
{
    for (i32 i = s.local_cnt-1; i >= 0; i--) {
        const Span other = s.symarr[s.locals[i]].span;
        if (span == other)            
            return s.locals[i];
    }
    for (i32 i = s.global_cnt-1; i >= 0; i--) {
        const Span other = s.symarr[s.globals[i]].span;        
        if (span == other) 
            return s.globals[i];
    }
    return -1;
}

static i32 declare_ident(SemaCtx &s, const Span span, const u32 flags)
{
    // TODO we can make this faster by only checking in the current scope
    // TODO we can make this more helpful in the case `self` is redeclared
    i32 id = resolve_ident(s, span);
    if (id != -1 && s.symarr[id].depth == s.depth)
        s.errarr.push(ErrMsg{span, "redeclared variable"});
    id = s.symarr.size();
    s.symarr.push(Symbol{.span = span, .flags = flags, .depth = s.depth, .idx = -1});
    // TODO error if more than 256 locals or 256 globals
    if (s.depth > 0) {
        s.locals[s.local_cnt] = id;
        s.local_cnt++;
    } else {
        s.globals[s.global_cnt] = id;
        s.global_cnt++;
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
static void update_captures(SemaCtx &s, const i32 id)
{
    Symbol &sym = s.symarr[id];
    FnDeclNode *fn = s.fn_node;
    FnDeclNode *parent = fn->parent;
    // NOTE: 
    // special cases. 
    // if sym->depth == 0 the ident is global so we don't need to capture it.
    // if id == fn->id then the fn we're compiling needs a ptr to itself,
    // we can get this from the current stack frame.
    if (sym.depth > s.symarr[fn->id].depth || sym.depth == 0 || id == fn->id)
        return;
    for (i32 i = 0; i < fn->stack_capture_cnt; i++) {
        if (id == fn->stack_captures[i])
            return;
    }
    for (i32 i = 0; i < fn->parent_capture_cnt; i++) {
        if (id == fn->parent_captures[i])
            return;
    } 
    sym.flags |= FLAG_CAPTURED; 
    // TODO handle more than 256 captures
    if (!parent || sym.depth > s.symarr[parent->id].depth || id == parent->id) {
        fn->stack_captures[fn->stack_capture_cnt] = id;
        fn->stack_capture_cnt++;
    } else {
        fn->parent_captures[fn->parent_capture_cnt] = id;
        fn->parent_capture_cnt++;
    }
}

static void analyze_node(SemaCtx &s, Node &node);

static void analyze_ident(SemaCtx &s, IdentNode &node)
{
    const i32 id = resolve_ident(s, node.span);
    if (id == -1) {
        s.errarr.push(ErrMsg{node.span, "not found in this scope"});
        return;
    }
    node.id = id;
    update_captures(s, id);
}

static void analyze_list(SemaCtx &s, const ListNode &node)
{
    for (i32 i = 0; i < node.cnt; i++)
        analyze_node(s, *node.items[i]);
}

static void analyze_unary(SemaCtx &s, const UnaryNode &node)
{
    analyze_node(s, *node.rhs);
}

static void analyze_binary(SemaCtx &s, const BinaryNode &node) 
{
    if (node.op_tag == TOKEN_EQ) {
        const bool ident = node.lhs->tag == NODE_IDENT;
        const bool dot = node.lhs->tag == NODE_PROPERTY 
            && static_cast<PropertyNode&>(*node.lhs).op_tag == TOKEN_DOT; 
        const bool list_elem = node.lhs->tag == NODE_BINARY 
            && static_cast<BinaryNode&>(*node.lhs).op_tag == TOKEN_L_SQUARE;
        if (!ident && !dot && !list_elem)
            s.errarr.push(ErrMsg{node.span, "cannot assign to left-hand expression"});
    }
    analyze_node(s, *node.lhs);
    analyze_node(s, *node.rhs);
}

static void analyze_property(SemaCtx &s, const PropertyNode &node)
{
    analyze_node(s, *node.lhs);
}

static void analyze_fn_call(SemaCtx &s, const CallNode &node) 
{
    analyze_node(s, *node.lhs);
    for (i32 i = 0; i < node.arity; i++)
        analyze_node(s, *node.args[i]);
}

static void analyze_block(SemaCtx &s, const BlockNode &node) 
{
    const i32 local_cnt = s.local_cnt;
    s.depth++;
    for (i32 i = 0; i < node.cnt; i++)
        analyze_node(s, *node.stmts[i]);
    s.depth--;
    s.local_cnt = local_cnt;
}

static void analyze_if(SemaCtx &s, const IfNode &node) 
{
    analyze_node(s, *node.cond);
    analyze_block(s, *node.thn);
    if (node.els)
        analyze_block(s, *node.els);
}

static void analyze_expr_stmt(SemaCtx &s, const ExprStmtNode &node) 
{
    analyze_node(s, *node.expr);
}

static void analyze_print(SemaCtx &s, const PrintNode &node) 
{
    analyze_node(s, *node.expr);
}

static void analyze_return(SemaCtx &s, const ReturnNode &node) 
{
    if (node.expr) {
        if (s.symarr[s.fn_node->id].flags & FLAG_INIT)
            s.errarr.push(ErrMsg{node.span, "init implicitly returns `self` so return cannot have expression"});
        analyze_node(s, *node.expr);
    }
}

static void analyze_var_decl(SemaCtx &s, VarDeclNode &node) 
{
    node.id = declare_ident(s, node.span, FLAG_NONE);
    if (node.init)
        analyze_node(s, *node.init);
}

static void analyze_fn_body(SemaCtx &s, FnDeclNode &node)
{
    // push state and enter the fn
    node.parent = s.fn_node;
    s.fn_node = &node;
    const i32 local_cnt = s.local_cnt;

    s.depth++;
    for (i32 i = 0; i < node.arity; i++)
        node.params[i].id = declare_ident(s, node.params[i].span, FLAG_NONE);
    s.depth--;
    analyze_block(s, *node.body);

    // pop state and leave the fn
    s.local_cnt = local_cnt;
    s.fn_node = node.parent;
    
    for (i32 i = 0; i < node.parent_capture_cnt; i++)
        update_captures(s, node.parent_captures[i]);
}

static void analyze_class_body(SemaCtx &s, const ClassDeclNode &node)
{
    bool has_init = false;
    const i32 local_cnt = s.local_cnt;
    s.depth++;

    for (i32 i = 0; i < node.cnt; i++) {
        FnDeclNode &fn_decl = static_cast<FnDeclNode&>(*node.methods[i]);        
        const Span fn_span = fn_decl.span;
        u32 flags = FLAG_NONE;
        if (fn_span == Span{"init", 4, 0}) {
            flags |= FLAG_INIT;
            has_init = true;
        }
        fn_decl.id = declare_ident(s, fn_span, flags);
        analyze_fn_body(s, *node.methods[i]);
    }

    s.local_cnt = local_cnt;
    s.depth--;
    // TODO allow implicit `init` methods
    if (!has_init)
        s.errarr.push(ErrMsg{node.span, "class must have `ini` method"});
}

static void analyze_node(SemaCtx &s, Node &node)
{
    switch (node.tag) {
    case NODE_ATOM:      return;
    case NODE_LIST:      analyze_list(s, static_cast<ListNode&>(node)); break;
    case NODE_IDENT:     analyze_ident(s, static_cast<IdentNode&>(node)); break;
    case NODE_UNARY:     analyze_unary(s, static_cast<UnaryNode&>(node)); break;
    case NODE_BINARY:    analyze_binary(s, static_cast<BinaryNode&>(node)); break;
    case NODE_PROPERTY:  analyze_property(s, static_cast<PropertyNode&>(node)); break;
    case NODE_CALL:      analyze_fn_call(s, static_cast<CallNode&>(node)); break;
    case NODE_VAR_DECL:  analyze_var_decl(s, static_cast<VarDeclNode&>(node)); break;
    case NODE_FN_DECL: {
        FnDeclNode &fn_decl = static_cast<FnDeclNode&>(node);
        fn_decl.id = declare_ident(s, fn_decl.span, FLAG_NONE);        
        analyze_fn_body(s, fn_decl);
        break;
    }
    case NODE_CLASS_DECL: {
        ClassDeclNode &class_decl = static_cast<ClassDeclNode&>(node);
        class_decl.id = declare_ident(s, class_decl.span, FLAG_NONE);        
        analyze_class_body(s, class_decl);
        break;
    }
    case NODE_EXPR_STMT: analyze_expr_stmt(s, static_cast<ExprStmtNode&>(node)); break;
    case NODE_BLOCK:     analyze_block(s, static_cast<BlockNode&>(node)); break;
    case NODE_IF:        analyze_if(s, static_cast<IfNode&>(node)); break;
    case NODE_RETURN:    analyze_return(s, static_cast<ReturnNode&>(node)); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     analyze_print(s, static_cast<PrintNode&>(node)); break;
    }
}

void SemaCtx::analyze(Node &node, Dynarr<Symbol> &symarr, Dynarr<ErrMsg> &errarr)
{
    SemaCtx s(node, symarr, errarr);

    // TODO I should probably be clearing the errorlist each time
    BlockNode& body = *static_cast<FnDeclNode&>(node).body;
    for (i32 i = 0; i < body.cnt; i++) {
        if (body.stmts[i]->tag == NODE_FN_DECL) {
            FnDeclNode &fn_decl = static_cast<FnDeclNode&>(*body.stmts[i]);
            fn_decl.id = declare_ident(s, fn_decl.span, FLAG_NONE);
        } else {
            ClassDeclNode &class_decl = static_cast<ClassDeclNode&>(*body.stmts[i]);
            class_decl.id = declare_ident(s, class_decl.span, FLAG_NONE);
        }
    } 
    for (i32 i = 0; i < body.cnt; i++) {
        if (body.stmts[i]->tag == NODE_FN_DECL) 
            analyze_fn_body(s, static_cast<FnDeclNode&>(*body.stmts[i]));        
        else
            analyze_class_body(s, static_cast<ClassDeclNode&>(*body.stmts[i]));
    }
}
