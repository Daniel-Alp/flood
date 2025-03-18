#pragma once
#include "compiler.h"
#define STACK_MAX 256

struct VM {
    struct Chunk chunk;
    Value value_stack[STACK_MAX];
    Value *stack_top;
    u8 *ip;
};

void init_vm(struct VM *vm, struct Chunk chunk);

void run(struct VM *vm);