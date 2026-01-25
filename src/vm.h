#pragma once
#include "chunk.h"
#define MAX_CALL_FRAMES (1024) // TODO implement tail call optimization
#define MAX_STACK       (MAX_CALL_FRAMES * 256)

struct ClosureObj;
struct ClassObj;

struct CallFrame {
    ClosureObj *closure;
    const u8 *ip;
    Value *bp;
};

enum InterpResultTag { INTERP_OK, INTERP_ERR };

struct InterpResult {
    InterpResultTag tag;
    union {
        const Value val;
        const char *message;
    };
};

struct VM {
    CallFrame *call_stack;
    u16 call_cnt;

    Value *val_stack;
    Value *sp;

    ClassObj *list_class;

    Dynarr<Value> globals;

    // linked list of all objects
    Obj *obj_list;
    Dynarr<Obj *> gray;

    VM();
    ~VM();
};

InterpResult runtime_err(const u8 *ip, VM &vm, const char *format, ...);

InterpResult run_vm(VM &vm, ClosureObj &closure);
