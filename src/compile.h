#pragma once
#include "sema.h"
#include "object.h"
#include "vm.h"

struct Compiler {
    struct FnDeclNode *fn_node;  // AST fn decl node we are inside of while traversing AST
    struct FnObj *fn;            // fn obj we are emitting bytecode into
    u32 fn_local_cnt;            // num locals in the current fn while traversing AST
    i32 main_fn_idx;             // idx into global array of main fn, or -1
    struct VM *vm;
    struct SymArr *sym_arr;
    struct ErrList errlist;
};  

void init_compiler(struct Compiler *compiler, struct SymArr *arr);

void release_compiler(struct Compiler *compiler);

struct ClosureObj *compile_file(struct VM *vm, struct Compiler *compiler, struct FnDeclNode *node);