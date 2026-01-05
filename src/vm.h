#pragma once
#include "arena.h"
#include "chunk.h"
#include "dynarr.h"
#define MAX_CALL_FRAMES (1024)   // TODO implement tail call optimization
#define MAX_STACK       (MAX_CALL_FRAMES * 256)

struct ClosureObj;

struct CallFrame {
    ClosureObj *closure;
    u8 *ip;
    Value *bp;
};

enum InterpResult {
    INTERP_COMPILE_ERR,
    INTERP_RUNTIME_ERR,
    INTERP_OK
};

struct VM {
    struct CallFrame *call_stack;
    u16 call_cnt;

    Value *sp;
    Value *val_stack;

    Dynarr<Value> globals;
    
    // linked list of all objects
    struct Obj *obj_list;
    u32 gray_cnt;
    u32 gray_cap;
    struct Obj **gray;
};

// void init_vm(struct VM *vm);

// void release_vm(struct VM *vm);

// void runtime_err(u8 *ip, struct VM *vm, const char *format, ...);

// enum InterpResult run_vm(struct VM *vm, struct ClosureObj *closure);
