#pragma once
#include "sema.h"
#include "object.h"
#include "vm.h"

struct Compiler {
    struct ErrList errlist;
    u32 stack_pos;
    u32 global_cnt;
    // global idx of the file's main fn (-1 if no main fn)
    i32 main_fn_idx;
    struct SymArr *sym_arr;
    // function body (or script) currently being compiled 
    struct FnObj *fn;
    // the VM we are compiling for
    struct VM *vm;
};

void init_compiler(struct Compiler *compiler, struct SymArr *arr);

void release_compiler(struct Compiler *compiler);

struct ClosureObj *compile_file(struct VM *vm, struct Compiler *compiler, struct FileNode *node);