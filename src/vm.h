#pragma once
#include "arena.h"
#include "chunk.h"
#include "dynarr.h"
#define MAX_CALL_FRAMES (1024) // TODO implement tail call optimization
#define MAX_STACK       (MAX_CALL_FRAMES * 256)

struct ClosureObj;
struct ClassObj;

struct CallFrame {
    ClosureObj *closure;
    const u8 *ip;
    Value *bp;
};

enum InterpResult { INTERP_COMPILE_ERR, INTERP_RUNTIME_ERR, INTERP_OK };

struct VM {
    // invariants:
    //      - call_stack[call_cnt-1]->closure is the currently executing closure
    //      - if call_cnt >= 2 then call_stack[call_cnt-2]->ip is return ip
    //      - if call_cnt >= 2 then call_stack[call_cnt-2]->bp is return bp
    //      - if runtime error occurs, every frame will have its ip set
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
