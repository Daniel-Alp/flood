#include <stdlib.h>
#include "sema.h"
#include "error.h"
#include "parse.h"
#include "scan.h"

static i32 declare_global(SemaCtx &s, const Span span, const u32 flags)
{
    for (i32 i = s.global_cnt-1; i >= 0; i--) {
        if (span == s.idarr[s.globals[i]].span) {
            s.errarr.push(ErrMsg{span, "redeclared variable"});
            return s.globals[i];
        }
    }
    const i32 id = s.idarr.len();
    const i32 idx = s.global_cnt;
    s.globals[s.global_cnt] = id;
    s.global_cnt++;
    s.idarr.push(Ident{.span = span, .flags = flags, .depth = s.depth, .idx = idx});
    return id;
}

static i32 declare_local(SemaCtx &s, const Span span, const u32 flags)
{
    for (i32 i = s.local_cnt-1; i >= 0; i--) {
        if (span == s.idarr[s.locals[i]].span) {
            s.errarr.push(ErrMsg{span, "redeclared variable"});
            return s.locals[i];
        }
    }
    for (i32 i = s.global_cnt-1; i >= 0; i--) {
        if (span == s.idarr[s.globals[i]].span) {
            s.errarr.push(ErrMsg{span, "redeclared variable"});
            return s.globals[i];
        }
    }    
    i32 i = s.local_cnt;
    for (; i-1 >= 0 && s.idarr[s.locals[i-1]].depth > s.idarr[s.fn_node->id].depth; i--) {
    }
    const i32 idx = s.local_cnt - i;
    s.locals[s.local_cnt] = s.idarr.len();
    s.local_cnt++;
    s.idarr.push(Ident{.span = span, .flags = flags, .depth = s.depth, .idx = idx});
    return s.idarr.len()-1;
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
static void propagate_captures(SemaCtx &s, const i32 id)
{
    Ident &ident = s.idarr[id];
    FnDeclNode *fn = s.fn_node;
    FnDeclNode *parent = fn->parent;
    // NOTE: 
    // special cases. 
    // if sym->depth == 0 the ident is global so we don't need to capture it.
    // if id == fn->id then the fn we're compiling needs a ptr to itself,
    // we can get this from the current stack frame.
    if (ident.depth > s.idarr[fn->id].depth || ident.depth == 0 || id == fn->id)
        return;
    for (i32 i = 0; i < fn->stack_capture_cnt; i++) {
        if (id == fn->stack_captures[i])
            return;
    }
    for (i32 i = 0; i < fn->parent_capture_cnt; i++) {
        if (id == fn->parent_captures[i])
            return;
    } 
    ident.flags |= FLAG_CAPTURED; 
    // TODO handle more than 256 captures
    if (!parent || ident.depth > s.idarr[parent->id].depth || id == parent->id) {
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
    for (i32 i = s.local_cnt-1; i >= 0; i--) {
        if (node.span == s.idarr[s.locals[i]].span) {
            node.id = s.locals[i];
            propagate_captures(s, node.id);
            return;
        }          
    }
    for (i32 i = s.global_cnt-1; i >= 0; i--) {
        if (node.span == s.idarr[s.globals[i]].span) {
            node.id = s.locals[i];
            return;
        }
    }
    s.errarr.push(ErrMsg{node.span, "not found in this scope"});
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
        if (s.idarr[s.fn_node->id].flags & FLAG_INIT)
            s.errarr.push(ErrMsg{node.span, "init implicitly returns `self` so return cannot have expression"});
        analyze_node(s, *node.expr);
    }
}

static void analyze_var_decl(SemaCtx &s, VarDeclNode &node) 
{
    node.id = declare_local(s, node.span, FLAG_NONE);
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
        node.params[i].id = declare_local(s, node.params[i].span, FLAG_NONE);
    s.depth--;
    analyze_block(s, *node.body);

    // pop state and leave the fn
    s.local_cnt = local_cnt;
    s.fn_node = node.parent;
    
    for (i32 i = 0; i < node.parent_capture_cnt; i++)
        propagate_captures(s, node.parent_captures[i]);
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
        fn_decl.id = declare_local(s, fn_decl.span, FLAG_NONE);        
        analyze_fn_body(s, fn_decl);
        break;
    }
    case NODE_EXPR_STMT: analyze_expr_stmt(s, static_cast<ExprStmtNode&>(node)); break;
    case NODE_BLOCK:     analyze_block(s, static_cast<BlockNode&>(node)); break;
    case NODE_IF:        analyze_if(s, static_cast<IfNode&>(node)); break;
    case NODE_RETURN:    analyze_return(s, static_cast<ReturnNode&>(node)); break;
    // TEMP remove when we add functions
    case NODE_PRINT:     analyze_print(s, static_cast<PrintNode&>(node)); break;
    // PROBABLY BETTER TO DO SOMETHING LIKE assert(false) or exit(1) TODO. see compile.cc for similar case 
    default:             s.errarr.push(ErrMsg{node.span, "default case of analyze_node reached"});
    }
}

void SemaCtx::analyze(ModuleNode &node, Dynarr<Ident> &idarr, Dynarr<ErrMsg> &errarr)
{
    SemaCtx s(idarr, errarr);

    // TODO I should probably be clearing the errorlist each time
    // BlockNode& body = *static_cast<FnDeclNode&>(node).body;
    for (i32 i = 0; i < node.cnt; i++) {
        if (node.decls[i]->tag == NODE_FN_DECL) {
            FnDeclNode &fn_decl = static_cast<FnDeclNode&>(*node.decls[i]);
            fn_decl.id = declare_global(s, fn_decl.span, FLAG_NONE);
        }
    } 
    for (i32 i = 0; i < node.cnt; i++) {
        if (node.decls[i]->tag == NODE_FN_DECL) 
            analyze_fn_body(s, static_cast<FnDeclNode&>(*node.decls[i]));
    }
}
