#include "sema.h"
#include "../libflood/dynarr.h"
#include "ast.h"

#include <stdio.h>

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
                errarr.push({node.span, "redeclared variable"});
                break;
            }
        }
        node.fn_depth = fn_depth;
        live_idents.push(&node);
    }

    void bubble_capture(DeclNode &node)
    {
        // if declared in function body or declared globally, don't capture
        if (node.fn_depth >= fn_depth || node.fn_depth == 0)
            return;
        for (i32 i = 0; i < fn_node->capture_cnt; i++) {
            if (&node == fn_node->captures[i].decl)
                return;
        }
        node.flags |= FLAG_CAPTURED;
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
        const i32 n_live_idents = live_idents.len();
        for (i32 i = 0; i < node.arity; i++)
            decl_ident(node.params[i]);

        node.body->local_cnt += node.arity;
        visit_block(*node.body);

        for (i32 i = live_idents.len(); i >= n_live_idents; i--)
            live_idents.pop();
        fn_node = saved_fn_node;
        fn_depth--;

        for (i32 i = 0; i < node.capture_cnt; i++) {
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
        for (i32 i = live_idents.len() - 1; i >= 0; i--) {
            if (live_idents[i]->span == node.span) {
                node.decl = live_idents[i];
                bubble_capture(*node.decl);
                return;
            }
        }
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
    // TODO use hashtable instead of dynarray
    // almost definitely faster, but implementing templated hashtable is difficult
    struct Assoc {
        DeclNode *decl;
        Loc loc;
    };
    Dynarr<Assoc> locs;
    i32 n_locals;
    i32 fn_depth;

    ResolveLoc() : n_locals(0), fn_depth(0){};

    Assoc *find_assoc(DeclNode *node)
    {
        // order of iteration should not matter but going backwards should be faster
        for (i32 i = locs.len() - 1; i >= 0; i--) {
            if (locs[i].decl == node)
                return &locs[i];
        }
        return nullptr;
    }

    void decl_local(DeclNode &node)
    {
        LocTag tag = node.flags & FLAG_CAPTURED ? LOC_STACK_HEAPVAL : LOC_LOCAL;
        printf("TAG IS %d\n", tag);
        printf("n_locals = %d\n", n_locals);
        locs.push({.decl = &node, .loc = {.tag = tag, .idx = n_locals}});
        n_locals++;
    }

    void visit_var_decl(VarDeclNode &node) override
    {
        decl_local(node);
    }

    void visit_fn_decl(FnDeclNode &node) override
    {
        if (fn_depth > 0)
            decl_local(node);

        Dynarr<Assoc> saved_locs;
        for (i32 i = 0; i < node.capture_cnt; i++) {
            Assoc *assoc = find_assoc(node.captures[i].decl);
            saved_locs.push(*assoc);
            node.captures[i].loc = assoc->loc;
            
            assoc->loc = {.tag = LOC_CAPTURED_HEAPVAL, .idx = i};
        }
        fn_depth++;
        const i32 saved_n_locals = n_locals;
        n_locals = 0;

        for (i32 i = 0; i < node.arity; i++)
            decl_local(node.params[i]);
        visit_block(*node.body);

        n_locals = saved_n_locals;
        fn_depth--;
        for (i32 i = 0; i < node.capture_cnt; i++)
            find_assoc(saved_locs[i].decl)->loc = saved_locs[i].loc;
    }

    void visit_ident(IdentNode &node) override
    {
        node.loc = find_assoc(node.decl)->loc;
    }

    void visit(ModuleNode &node)
    {
        for (i32 i = 0; i < node.cnt; i++) {
            if (node.decls[i]->tag == NODE_FN_DECL || node.decls[i]->tag == NODE_CLASS_DECL)
                locs.push({.decl = *node.decls, .loc = {.tag = LOC_GLOBAL, .idx = i}});
        }
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.decls[i]);
    }
};

void analyze(ModuleNode &node, Dynarr<ErrMsg> &errarr)
{
    ResolveIdents pass0(errarr);
    pass0.visit(node);
    if (errarr.len() > 0)
        return;
    ResolveLoc pass1;
    pass1.visit(node);
}
