#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "object.h"
#include "debug.h"

static u32 get_opcode_line(u32 *lines, u32 tgt_opcode_idx)
{
    // see chunk.h
    u32 opcode_idx = 0;
    i32 i = 1;
    while (true) {
        opcode_idx += lines[i];
        if (opcode_idx >= tgt_opcode_idx)
            return lines[i-1];
        i += 2;
    }
}

static void runtime_err(struct VM *vm, const char *msg) 
{
    printf("%s\n", msg);
    for (i32 i = vm->call_cnt-1; i >= 1; i--) {
        struct CallFrame frame = vm->call_stack[i];
        u32 line = get_opcode_line(frame.fn->chunk.lines, frame.ip-1 - frame.fn->chunk.code);
        printf("[line %d] in %s\n", line, frame.fn->name);
    }
}

struct Obj *alloc_vm_obj(struct VM *vm, u64 size)
{
    struct Obj *obj = allocate(size);
    obj->next = vm->obj_list;
    obj->color = GC_WHITE;
    obj->printed = 0;
    vm->obj_list = obj;
    return obj;
}

void release_vm_obj(struct VM *vm)
{
    while (vm->obj_list) {
        struct Obj *obj = vm->obj_list;
        switch(obj->tag) {
        case OBJ_FN:   release_fn_obj((struct FnObj*)obj); break;
        case OBJ_LIST: release_list_obj((struct ListObj*)obj); break;
        }
        vm->obj_list = vm->obj_list->next;
        release(obj);
    }
}

void init_vm(struct VM *vm)
{
    vm->sp = vm->val_stack;
    init_val_array(&vm->globals);
    vm->obj_list = NULL;

    vm->gray_cnt = 0;
    vm->gray_cap = 8;
    vm->gray = allocate(vm->gray_cap * sizeof(struct Obj*));
}

void release_vm(struct VM *vm) 
{
    vm->sp = NULL;
    release_val_array(&vm->globals);
    release_vm_obj(vm);

    vm->gray_cnt = 0;
    vm->gray_cap = 0;
    release(vm->gray);
    vm->gray = NULL;
}

// TODO check if exceeding max stack size
enum InterpResult run_vm(struct VM *vm, struct FnObj *fn) 
{
    Value *sp = vm->val_stack;

    struct CallFrame *frame = vm->call_stack;
    frame->fn = fn;
    frame->bp = sp;
    vm->call_cnt = 1;
    register u8 *ip = frame->fn->chunk.code;
    while(true) {
        u8 op = *ip;
        ip++;
        switch (op) {
        case OP_NIL: {
            sp[0] = MK_NIL;
            sp++;
            break;
        }
        case OP_TRUE: {
            sp[0] = MK_BOOL(true);
            sp++;
            break;
        }
        case OP_FALSE: {
            sp[0] = MK_BOOL(false);
            sp++;
            break;
        }
        case OP_ADD: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs)+AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_SUB: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs)-AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_MUL: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs)*AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_DIV: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs)/AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_FLOORDIV: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(floor(AS_NUM(lhs)/AS_NUM(rhs)));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_MOD: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(fmod(AS_NUM(lhs),AS_NUM(rhs)));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LT: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs)<AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs)<=AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_GT: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs)>AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_GEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs)>=AS_NUM(rhs));
                sp--;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_EQEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            sp[-2] = MK_BOOL(val_eq(lhs, rhs));
            sp--;
            break;
        }
        case OP_NEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            sp[-2] = MK_BOOL(!val_eq(lhs, rhs));
            sp--;
            break;
        }
        case OP_NEGATE: {
            Value val = sp[-1];
            if (IS_NUM(val)) {
                sp[-1] = MK_NUM(-AS_NUM(val));
            } else {
                frame->ip = ip;
                runtime_err(vm, "operand must be number");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_NOT: {
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                sp[-1] = MK_BOOL(!AS_BOOL(val));
            } else {
                frame->ip = ip;
                runtime_err(vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LIST: {
            u8 cnt = *ip++;
            sp -= cnt;
            Value *vals = sp;
            struct Obj *list = alloc_vm_obj(vm, sizeof(struct ListObj));
            // TODO benchmark, maybe inline
            init_list_obj((struct ListObj*)list, vals, cnt);
            sp[0] = MK_OBJ(list);
            sp++;
            break;
        }
        case OP_GET_CONST: {
            u16 idx = *ip++;
            sp[0] = frame->fn->chunk.constants.vals[idx];
            sp++;
            break;
        }
        case OP_GET_LOCAL: {
            u8 idx = *ip++;
            sp[0] = frame->bp[idx];
            sp++;
            break;
        }
        case OP_GET_SUBSCR: {
            Value container = sp[-2];
            Value idx = sp[-1];
            if (IS_LIST(container)) {
                if (IS_NUM(idx)) {
                    if (AS_NUM(idx) >= 0 && AS_NUM(idx) < AS_LIST(container)->cnt) {
                        sp[-2] = AS_LIST(container)->vals[(u32)AS_NUM(idx)];
                        sp--;
                    } else {
                        frame->ip = ip;
                        // TODO let runtime_err take var args
                        runtime_err(vm, "list index out of bounds");
                        return INTERP_RUNTIME_ERR;
                    }
                } else {
                    frame->ip = ip;
                    runtime_err(vm, "list index must be number");
                    return INTERP_RUNTIME_ERR;
                }
            } else {
                frame->ip = ip;
                runtime_err(vm, "object is not subscriptable");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_SET_SUBSCR: {
            Value container = sp[-3];
            Value idx = sp[-2];
            Value val = sp[-1];
            if (IS_LIST(container)) {
                if (IS_NUM(idx)) {
                    if (AS_NUM(idx) >= 0 && AS_NUM(idx) < AS_LIST(container)->cnt) {
                        AS_LIST(container)->vals[(u32)AS_NUM(idx)] = val;
                        // TODO consider making assignment a statement rather than an expression
                        sp[-3] = sp[-1];
                        sp -= 2;
                    } else {
                        frame->ip = ip;
                        // TODO let runtime_err take var args
                        runtime_err(vm, "list index out of bounds");
                        return INTERP_RUNTIME_ERR;
                    }
                } else {
                    frame->ip = ip;
                    runtime_err(vm, "list index must be number");
                    return INTERP_RUNTIME_ERR;
                }
            } else {
                frame->ip = ip;
                runtime_err(vm, "object is not subscriptable");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_SET_LOCAL: {
            u8 idx = *ip++;
            frame->bp[idx] = sp[-1];
            break;
        }
        case OP_GET_GLOBAL: {
            u8 idx = *ip++;
            sp[0] = vm->globals.vals[idx];
            sp++;
            break;
        }
        case OP_SET_GLOBAL: {
            u8 idx = *ip++;
            vm->globals.vals[idx] = sp[-1];
            break;
        }
        case OP_JUMP: {
            ip += ((*ip++) << 8) + (*ip++);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            u16 offset = ((*ip++) << 8) + (*ip++);
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                if (!AS_BOOL(val))
                    ip += offset;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_JUMP_IF_TRUE: {
            u16 offset = ((*ip++) << 8) + (*ip++);
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                if (AS_BOOL(val))
                    ip += offset;
            } else {
                frame->ip = ip;
                runtime_err(vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_CALL: {
            u8 arg_count = *ip++;
            Value val = sp[-1-arg_count];
            if (IS_FN(val)) {
                struct FnObj *fn = AS_FN(val);
                if (fn->arity != arg_count) {
                    frame->ip = ip;
                    runtime_err(vm, "incorrect number of arguments provided");
                    return INTERP_RUNTIME_ERR;
                }
                if (vm->call_cnt+1 >= MAX_CALL_FRAMES) {
                    frame->ip = ip;
                    runtime_err(vm, "stack overflow");
                    return INTERP_RUNTIME_ERR;
                }
                frame->ip = ip; 
                
                vm->call_cnt++;
                frame++;
                frame->bp = sp-arg_count;
                frame->fn = fn;
                ip = fn->chunk.code;
            } else {
                frame->ip = ip;
                runtime_err(vm, "attempt to call non-function");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_RETURN: {
            vm->call_cnt--;
            if (vm->call_cnt == 0)
                return INTERP_OK;
            // when a function returns the stack looks like this
            //      <function foo> 
            // bp-> <arg>
            //      ...
            //      <arg>
            //      <local>
            //      ...
            //      <local> <-return value
            // sp-> 
            frame->bp[-1] = sp[-1]; // move return value 
            sp = frame->bp; // pop locals
            frame--; 
            ip = frame->ip;
            break;
        }
        case OP_POP: {
            sp--;
            break;
        }
        case OP_POP_N: {
            u8 n = *ip++;
            sp -= n;
            break;
        }
        case OP_PRINT: {
            print_val(sp[-1]);
            printf("\n");
            sp--;
            break;
        }
        }
        vm->sp = sp;
        // printf("%s\n", opcode_str[op]);
        // print_stack(vm, sp, frame->bp);
        // TODO run garbage collector only if it exceeds threshold
        // run it each iteration only if we define smth
        collect_garbage(vm);
    }
}
