#pragma once
#include "chunk.h"
#include "symbol.h"
#define MAX_CALL_FRAMES (64)
#define MAX_STACK       (MAX_CALL_FRAMES * 256)

struct CallFrame {
    struct FnObj *fn;
    u8 *ip;
    Value *bp;
};

struct ModArray {
    u32 cap;
    u32 cnt;
    struct Span *names;
    struct SymMap *sym_maps;
};

struct VM {
    struct CallFrame call_stack[MAX_CALL_FRAMES];
    u16 call_count;

    Value val_stack[MAX_STACK];
    // stack pointer is stored in the run_vm function

    struct ValArray globals;
    // NOT IN USE CURRENTLY
    struct ModArray modules; // array associating module name with its globals
};

void init_vm(struct VM *vm);

void release_vm(struct VM *vm);

void run_vm(struct VM *vm, struct FnObj *fn);