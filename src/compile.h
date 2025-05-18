#pragma once
#include "sema.h"
#include "object.h"

struct Compiler {
    u32 stack_pos;
    u32 global_cnt;
    struct SymArr *sym_arr;
    // function body (or script) currently being compiled 
    struct FnObj *fn;
};

void init_compiler(struct Compiler *compiler, struct SymArr *arr);

void release_compiler(struct Compiler *compiler);

struct FnObj *compile_module(struct Compiler *compiler, struct ModuleNode *node);