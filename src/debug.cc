#include "debug.h"
#include "ast.h"
#include <stdio.h>

const char *loc_tag_str(const LocTag tag)
{
    // clang-format off
    switch(tag) {
    case LOC_LOCAL:             return "local";
    case LOC_GLOBAL:            return "global";
    case LOC_STACK_HEAPVAL:     return "stack_heapval";
    case LOC_CAPTURED_HEAPVAL:  return "captured_heapval";
    }
    // clang-format on
    return nullptr;
}

struct AstPrinter final : public AstVisitor {
    i32 offset;
    const bool verbose;
    AstPrinter(const bool verbose) : offset(0), verbose(verbose) {}

    void visit_atom(AtomNode &node) override
    {
        printf("Atom %.*s", node.span.len, node.span.start);
    }

    void visit_list(ListNode &node) override
    {
        printf("List");
        for (i32 i = 0; i < node.cnt; i++)
            visit_expr(*node.items[i]);
    }

    void visit_ident(IdentNode &node) override
    {
        printf("Ident %.*s", node.span.len, node.span.start);
        if (verbose)
            printf(": %p", node.decl);
    }

    void visit_unary(UnaryNode &node) override
    {
        printf("Unary\n");
        printf("%*s", offset, "");
        printf("%.*s", node.span.len, node.span.start);
        visit_expr(*node.rhs);
    }

    void visit_binary(BinaryNode &node) override
    {
        printf("Binary\n");
        printf("%*s", offset, "");
        printf("%.*s", node.span.len, node.span.start);
        visit_expr(*node.lhs);
        visit_expr(*node.rhs);
    }

    void visit_selector(SelectorNode &node) override
    {
        printf("Selector");
        visit_expr(*node.lhs);
        printf("\n%*s", offset, "");
        printf("%.*s", node.sym.len, node.sym.start);
    }

    void visit_subscr(SubscrNode &node) override
    {
        printf("Subscr");
        visit_expr(*node.lhs);
        visit_expr(*node.rhs);
    }

    void visit_assign(AssignNode &node) override
    {
        printf("Assign\n");
        printf("%*s", offset, "");
        printf("%.*s", node.span.len, node.span.start);
        visit_expr(*node.lhs);
        visit_expr(*node.rhs);
    }

    void visit_call(CallNode &node) override
    {
        printf("Call");
        visit_expr(*node.lhs);
        for (i32 i = 0; i < node.arity; i++)
            visit_expr(*node.args[i]);
    }

    void visit_expr(Node &node) override
    {
        printf("\n%*s(", offset, "");
        offset += 2;
        AstVisitor::visit_expr(node);
        printf(")");
        offset -= 2;
    }

    void visit_var_decl(VarDeclNode &node) override
    {
        printf("Var %.*s", node.span.len, node.span.start);
        if (verbose)
            printf(": %p, %s, %d", &node, loc_tag_str(node.loc.tag), node.loc.idx);
        if (node.init)
            visit_expr(*node.init);
    }

    void visit_fn_decl(FnDeclNode &node) override
    {
        printf("Fn %.*s(", node.span.len, node.span.start);
        for (i32 i = 0; i < node.arity - 1; i++) {
            const VarDeclNode param = node.params[i];
            printf("%.*s, ", param.span.len, param.span.start);
        }
        if (node.arity > 0) {
            const VarDeclNode param = node.params[node.arity - 1];
            printf("%.*s", param.span.len, param.span.start);
        }
        printf(")");
        if (verbose)
            printf(": %p, %s, %d", &node, loc_tag_str(node.loc.tag), node.loc.idx);
        if (verbose && node.capture_cnt > 0) {
            for (i32 i = 0; i < node.capture_cnt - 1; i++) {
                const CaptureDecl *capt = node.captures[i];
                printf("\n%*s", offset, "");
                printf("(Capture %.*s: %p, %s, %d)", capt->span.len, capt->span.start, 
                    capt, loc_tag_str(capt->loc.tag), capt->loc.idx);
            }
            const CaptureDecl *capt = node.captures[node.capture_cnt - 1];
            printf("\n%*s", offset, "");
            printf("(Capture %.*s: %p, %s, %d)", capt->span.len, capt->span.start, 
                capt, loc_tag_str(capt->loc.tag), capt->loc.idx);
        }
        visit_stmt(*node.body);
    }

    void visit_class_decl(ClassDeclNode &node) override
    {
        printf("Class %.*s", node.span.len, node.span.start);
        if (verbose)
            printf(": %p, %s, %d", &node, loc_tag_str(node.loc.tag), node.loc.idx);
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.methods[i]);
    }

    void visit_expr_stmt(ExprStmtNode &node) override
    {
        printf("ExprStmt");
        visit_expr(*node.expr);
    }

    void visit_return(ReturnNode &node) override
    {
        printf("Return");
        if (node.expr)
            visit_expr(*node.expr);
    }

    void visit_block(BlockNode &node) override
    {
        printf("Block");
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.stmts[i]);
    }

    void visit_if(IfNode &node) override
    {
        printf("If");
        visit_expr(*node.cond);
        visit_stmt(*node.thn);
        if (node.els)
            visit_stmt(*node.els);
    }

    void visit_print(PrintNode &node) override
    {
        printf("Print");
        visit_expr(*node.expr);
    }

    void visit_stmt(Node &node) override
    {
        printf("\n%*s(", offset, "");
        offset += 2;
        AstVisitor::visit_stmt(node);
        printf(")");
        offset -= 2;
    }

    // TODO make visit a method in AstVisitor?
    void visit(ModuleNode &node)
    {
        printf("(Module");
        offset += 2;
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.decls[i]);
        offset -= 2;
        printf(")\n");
    }
};

void print_module(ModuleNode &node, const bool verbose)
{
    AstPrinter ast_printer(verbose);
    ast_printer.visit(node);
}