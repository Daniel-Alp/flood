// #pragma once
// #include "dynarr.h"
// #include "error.h"
// #include "parse.h"
// #include "sema.h"
// #include "object.h"
// #include "vm.h"

// struct CompileCtx {
//     const FnDeclNode *fn_node; // AST fn decl node we are inside of while traversing AST
//     FnObj *fn;                 // fn obj we are emitting bytecode into
//     i32 fn_local_cnt;          // num locals in the current fn while traversing AST
//     i32 main_fn_idx;           // idx into global array of main fn, or -1
    
//     VM &vm;
//     // TODO this should be const but for some reason were modifying the symarr while traversing the AST
//     // I am not a fan of this
//     Dynarr<Symbol> &symarr;
//     Dynarr<ErrMsg> &errarr;

//     Chunk &chunk()
//     {
//         return fn->chunk;
//     }

//     static ClosureObj &compile(VM &vm, Dynarr<Symbol> &symarr, const Node &node, Dynarr<ErrMsg> &errarr);

// private:
//     CompileCtx(VM &vm, Dynarr<Symbol> &symarr, Node &node, Dynarr<ErrMsg> &errarr)
//         : vm(vm), symarr(symarr), errarr(errarr), fn_node(static_cast<const FnDeclNode*>(&node))
//     {}
// };
