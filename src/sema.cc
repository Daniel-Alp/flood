#include "sema.h"
#include "../libflood/arena.h"
#include "../libflood/dynarr.h"
#include "ast.h"

struct ResolveIdents final : public AstVisitor {
    Dynarr<DeclNode *> live_idents;
    Dynarr<FnDeclNode *> fn_nodes;
    Dynarr<ErrMsg> &errarr;
    Arena &arena;

    ResolveIdents(Dynarr<ErrMsg> &errarr, Arena &arena) : errarr(errarr), arena(arena){};

    void decl_ident(DeclNode &node)
    {
        for (i32 i = live_idents.len() - 1; i >= 0; i--) {
            if (live_idents[i]->span == node.span) {
                errarr.push({node.span, "redeclared variable"});
                break;
            }
        }
        node.fn_depth = fn_nodes.len();
        live_idents.push(&node);
    }

    DeclNode *resolve_ident_rec(Span span, const i32 fn_depth, i32 i)
    {
        // if fn_depth > 0, check in fn_nodes[fn_depth-1] locals
        // if fn_depth = 0, check in globals
        for (; i >= 0 && live_idents[i]->fn_depth >= fn_depth; i--) {
            if (live_idents[i]->span == span)
                return live_idents[i];
        }
        if (fn_depth == 0)
            return nullptr;
        // check function's captures
        FnDeclNode *node = fn_nodes[fn_depth - 1];
        for (i32 i = 0; i < node->capture_cnt; i++) {
            if (node->captures[i]->span == span)
                return node->captures[i];
        }
        // check in parent
        DeclNode *decl = resolve_ident_rec(span, fn_depth - 1, i);
        if (decl == nullptr || decl->fn_depth == 0)
            return decl;
        decl->flags |= FLAG_CAPTURED;
        node->captures[node->capture_cnt] = alloc<CaptureDecl>(arena, span, decl);
        node->captures[node->capture_cnt]->fn_depth = fn_depth;
        node->capture_cnt++;
        return node->captures[node->capture_cnt - 1];
    }

    DeclNode *resolve_ident(Span span)
    {
        return resolve_ident_rec(span, fn_nodes.len(), live_idents.len() - 1);
    }

    void visit_var_decl(VarDeclNode &node) override
    {
        AstVisitor::visit_var_decl(node);
        decl_ident(node);
    }

    void visit_fn_decl(FnDeclNode &node) override
    {
        // fn_depth == 0 if node is a method (do not declare ident) or global (already declared ident)
        if (fn_nodes.len() > 0)
            decl_ident(node);

        fn_nodes.push(&node);
        const i32 n_live_idents = live_idents.len();
        for (i32 i = 0; i < node.arity; i++)
            decl_ident(node.params[i]);

        node.body->local_cnt += node.arity;
        visit_block(*node.body);

        for (i32 i = live_idents.len(); i > n_live_idents; i--)
            live_idents.pop();
        fn_nodes.pop();
    }

    void visit_class_decl(ClassDeclNode &node) override
    {
        Dynarr<Span> method_spans;
        for (i32 i = 0; i < node.cnt; i++) {
            FnDeclNode &method_decl = *node.methods[i];
            for (i32 i = method_spans.len() - 1; i >= 0; i--) {
                if (method_decl.span == method_spans[i]) {
                    errarr.push({method_decl.span, "redeclared method"}); // TODO test
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
        node.decl = resolve_ident(node.span);
        if (node.decl == nullptr)
            errarr.push({node.span, "not found in this scope"}); // TODO change error message
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

struct ResolveLoc final : public AstVisitor {
    i32 n_locals;
    i32 fn_depth;

    ResolveLoc() : n_locals(0), fn_depth(0){};

    void decl_local(DeclNode &node)
    {
        node.loc = {
            .tag = node.flags & FLAG_CAPTURED ? LOC_STACK_HEAPVAL : LOC_LOCAL,
            .idx = n_locals,
        };
        n_locals++;
    }

    void visit_var_decl(VarDeclNode &node) override
    {
        AstVisitor::visit_var_decl(node);
        decl_local(node);
    }

    void visit_fn_decl(FnDeclNode &node) override
    {
        if (fn_depth > 0)
            decl_local(node);
        for (i32 i = 0; i < node.capture_cnt; i++)
            node.captures[i]->loc = {.tag = LOC_CAPTURED_HEAPVAL, .idx = i};
        fn_depth++;
        const i32 saved_n_locals = n_locals;
        n_locals = 0;
        for (i32 i = 0; i < node.arity; i++)
            decl_local(node.params[i]);
        visit_block(*node.body);
        n_locals = saved_n_locals;
        fn_depth--;
    }

    void visit_block(BlockNode &node) override
    {
        const i32 saved_n_locals = n_locals;
        AstVisitor::visit_block(node);
        n_locals = saved_n_locals;
    }

    void visit(ModuleNode &node)
    {
        for (i32 i = 0; i < node.cnt; i++) {
            if (node.decls[i]->tag == NODE_FN_DECL || node.decls[i]->tag == NODE_CLASS_DECL)
                node.decls[i]->loc = {.tag = LOC_GLOBAL, .idx = i};
        }
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.decls[i]);
    }
};

void analyze(ModuleNode &node, Dynarr<ErrMsg> &errarr, Arena &arena)
{
    ResolveIdents pass0(errarr, arena);
    pass0.visit(node);
    if (errarr.len() > 0)
        return;
    ResolveLoc pass1;
    pass1.visit(node);
}
