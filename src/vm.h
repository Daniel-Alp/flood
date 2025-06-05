#pragma once
#include "arena.h"
#include "chunk.h"
#include "symbol.h"
#define MAX_CALL_FRAMES (64)
#define MAX_STACK       (MAX_CALL_FRAMES * 256)

struct CallFrame {
    struct FnObj *fn;
    u8 *ip;
    Value *bp;
};

enum InterpResult {
    INTERP_COMPILE_ERR,
    INTERP_RUNTIME_ERR,
    INTERP_OK
};

struct VM {
    struct CallFrame call_stack[MAX_CALL_FRAMES];
    u16 call_cnt;

    Value *sp;
    Value val_stack[MAX_STACK];
    // stack pointer is stored in the run_vm function

    struct ValArray globals;
    
    // linked list of all objects
    struct Obj *obj_list;
    u32 gray_cnt;
    u32 gray_cap;
    struct Obj **gray;
};

void init_vm(struct VM *vm);

void release_vm(struct VM *vm);

enum InterpResult run_vm(struct VM *vm, struct FnObj *fn);

struct Obj *alloc_vm_obj(struct VM *vm, u64 size);