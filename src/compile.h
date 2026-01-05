#pragma once
#include "dynarr.h"
#include "error.h"
#include "object.h"
#include "parse.h"
#include "sema.h"

struct CompileCtx {
    const FnDeclNode *fn_node; // AST fn decl node we are inside of while traversing AST
    FnObj *fn;                 // fn obj we are emitting bytecode into while traversing AST

    VM &vm;
    const Dynarr<Ident> &idarr;
    Dynarr<ErrMsg> &errarr;

    Chunk &chunk()
    {
        return fn->chunk;
    }

    static ClosureObj *compile(VM &vm, const Dynarr<Ident> &idarr, const ModuleNode &node, Dynarr<ErrMsg> &errarr);

private:
    CompileCtx(VM &vm, const Dynarr<Ident> &idarr, Dynarr<ErrMsg> &errarr)
        : fn_node(nullptr), fn(nullptr), vm(vm), idarr(idarr), errarr(errarr)
    {
    }
};
