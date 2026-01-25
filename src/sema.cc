#include "sema.h"
#include "ast.h"
#include "dynarr.h"
#include "error.h"
#include "scan.h"

struct ResolveIdents final : public AstVisitor {
    Dynarr<DeclNode *> live_idents;
    i32 fn_depth;
    FnDeclNode *fn_node;
    Dynarr<ErrMsg> &errarr;

    ResolveIdents(Dynarr<ErrMsg> &errarr) : fn_depth(0), fn_node(nullptr), errarr(errarr){};

    void decl_ident(DeclNode &node)
    {
        for (i32 i = live_idents.len() - 1; i >= 0; i--) {
            if (live_idents[i]->span == node.span) {
                errarr.push(ErrMsg{node.span, "redeclared variable"});
                return;
            }
        }
        node.fn_depth = fn_depth;
        live_idents.push(&node);
    }

    void bubble_capture(DeclNode &node)
    {
        // if declared in function body or declared globally, don't capture
        if (node.fn_depth > fn_depth || node.fn_depth == 0)
            return;
        for (i32 i = 0; i < fn_node->capture_cnt; i++) {
            if (&node == fn_node->captures[i].decl)
                return;
        }
        fn_node->captures[fn_node->capture_cnt] = {
            .decl = &node,
            .loc =
                {
                    .tag = node.fn_depth == fn_depth - 1 ? LOC_STACK_HEAPVAL : LOC_CAPTURED_HEAPVAL,
                    .idx = 0,
                },
        };
        fn_node->capture_cnt++;
    }

    void visit_var_decl(VarDeclNode &node) override
    {
        decl_ident(node);
    }

    void visit_fn_decl(FnDeclNode &node) override
    {
        // fn_depth == 0 if node is a method (do not declare ident) or global (already declared ident)
        if (fn_depth > 0)
            decl_ident(node);

        fn_depth++;
        FnDeclNode *saved_fn_node = fn_node;
        fn_node = &node;

        for (i32 i = 0; i < node.arity; i++)
            decl_ident(node.params[i]);
        node.body->local_cnt += node.arity;
        visit_block(*node.body);
        for (i32 i = 0; i < node.arity; i++)
            live_idents.pop();

        fn_node = saved_fn_node;
        fn_depth--;
        if (!fn_node)
            return;
        for (i32 i = 0; i < fn_node->capture_cnt; i++) {
            if (node.captures[i].loc.tag == LOC_CAPTURED_HEAPVAL)
                bubble_capture(*node.captures[i].decl);
        }
    }

    void visit_class_decl(ClassDeclNode &node) override
    {
        Dynarr<Span> method_spans;
        for (i32 i = 0; i < node.cnt; i++) {
            FnDeclNode &method_decl = *node.methods[i];
            for (i32 i = method_spans.len() - 1; i >= 0; i--) {
                if (method_decl.span == method_spans[i]) {
                    errarr.push(ErrMsg{method_decl.span, "redeclared method"}); // TODO test
                    break;
                }
            }
            method_spans.push(method_decl.span);
            method_decl.flags |= FLAG_METHOD;
            if (method_decl.span == "init")
                method_decl.flags |= FLAG_INIT;
            visit_fn_decl(method_decl);
        }
    }

    void visit_ident(IdentNode &node) override
    {
        for (i32 i = live_idents.len() - 1; i >= 0; i--) {
            if (live_idents[i]->span == node.span) {
                node.decl = live_idents[i];
                bubble_capture(*node.decl);
                return;
            }
        }
        errarr.push(ErrMsg{node.span, "not found in this scope"}); // TODO change error message
    }

    void visit_block(BlockNode &node) override
    {
        const i32 n_live_idents = live_idents.len();
        AstVisitor::visit_block(node);
        node.local_cnt += live_idents.len() - n_live_idents;
        while (live_idents.len() > n_live_idents)
            live_idents.pop();
    }

    void visit(ModuleNode &node)
    {
        for (i32 i = 0; i < node.cnt; i++)
            decl_ident(*node.decls[i]);
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.decls[i]);
    }
};


void analyze(ModuleNode &node, Dynarr<ErrMsg> &errarr)
{
    ResolveIdents pass0(errarr);
    pass0.visit(node);
}
