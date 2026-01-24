#include "ast.h"

void AstVisitor::visit_atom(AtomNode &node)
{
}

void AstVisitor::visit_list(ListNode &node)
{
    for (i32 i = 0; i < node.cnt; i++) 
        visit_expr(*node.items[i]);
}

void AstVisitor::visit_ident(IdentNode &node)
{
}

void AstVisitor::visit_unary(UnaryNode &node)
{
    visit_expr(*node.rhs);
}

void AstVisitor::visit_binary(BinaryNode &node)
{
    visit_expr(*node.lhs);
    visit_expr(*node.rhs);
}

void AstVisitor::visit_selector(SelectorNode &node)
{
    visit_expr(*node.lhs);
}

void AstVisitor::visit_subscr(SubscrNode &node)
{
    visit_expr(*node.lhs);
    visit_expr(*node.rhs);
}

void AstVisitor::visit_assign(AssignNode &node)
{
    visit_expr(*node.lhs);
    visit_expr(*node.rhs);
}

void AstVisitor::visit_call(CallNode &node)
{
    visit_expr(*node.lhs);
    for (i32 i = 0; i < node.arity; i++)
        visit_expr(*node.args[i]);
}

void AstVisitor::visit_expr(Node &node)
{
    // clang-format off
    switch(node.tag) {
    case NODE_ATOM:     visit_atom(static_cast<AtomNode&>(node)); return;
    case NODE_LIST:     visit_list(static_cast<ListNode&>(node)); return;
    case NODE_UNARY:    visit_unary(static_cast<UnaryNode&>(node)); return;
    case NODE_BINARY:   visit_binary(static_cast<BinaryNode&>(node)); return;
    case NODE_SELECTOR: visit_selector(static_cast<SelectorNode&>(node)); return;
    case NODE_SUBSCR:   visit_subscr(static_cast<SubscrNode&>(node)); return;
    case NODE_ASSIGN:   visit_assign(static_cast<AssignNode&>(node)); return;
    case NODE_CALL:     visit_call(static_cast<CallNode&>(node)); return;
    default:            return; // TODO assert(false)?
    }
    // clang-format on
}

void AstVisitor::visit_var_decl(VarDeclNode &node)
{
    if (node.init)
        visit_expr(*node.init);
}

void AstVisitor::visit_class_decl(ClassDeclNode &node)
{
    for (i32 i = 0; i < node.cnt; i++)
        visit_fn_decl(*node.methods[i]);
}

void AstVisitor::visit_expr_stmt(ExprStmtNode &node)
{
    visit_expr(*node.expr);
}

void AstVisitor::visit_block(BlockNode &node)
{
    for (i32 i = 0; i < node.cnt; i++)
        visit_stmt(*node.stmts[i]);
}

void AstVisitor::visit_if(IfNode &node)
{
    visit_expr(*node.cond);
    visit_block(*node.thn);
    visit_block(*node.els);
}

void AstVisitor::visit_print(PrintNode &node)
{
    visit_expr(*node.expr);
}

void AstVisitor::visit_stmt(Node &node)
{
    // clang-format off
    switch(node.tag) {
    case NODE_VAR_DECL:   visit_var_decl(static_cast<VarDeclNode&>(node)); return;
    case NODE_FN_DECL:    visit_fn_decl(static_cast<FnDeclNode&>(node)); return;
    case NODE_CLASS_DECL: visit_class_decl(static_cast<ClassDeclNode&>(node)); return;
    case NODE_EXPR_STMT:  visit_expr_stmt(static_cast<ExprStmtNode&>(node)); return;
    case NODE_IF:         visit_if(static_cast<IfNode&>(node)); return;
    case NODE_PRINT:      visit_print(static_cast<PrintNode&>(node)); return;
    default:              return; // TODO assert(false)?
    }
    // clang-format on
}
