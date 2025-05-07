#pragma once
#include "chunk.h"
#include "sema.h"

struct Compiler {
    // tracks size of value stack
    u32 stack_count;
    struct Chunk chunk;
};

void init_compiler(struct Compiler *compiler);

void release_compiler(struct Compiler *compiler);

void compile(struct Compiler *compiler, struct Node *node, struct SymTable *st);