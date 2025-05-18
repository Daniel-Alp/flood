#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "memory.h"
#include "object.h"
#include "debug.h"

// TODO fix memory leaks

static void init_mod_array(struct ModArray *modules) 
{
    modules->cnt = 0;
    modules->cap = 8;
    modules->names = allocate(modules->cap * sizeof(struct Span));
    modules->sym_maps = allocate(modules->cap * sizeof(struct SymMap));
}

static void release_mod_array(struct ModArray *modules) 
{
    modules->cnt = 0;
    modules->cap = 0;
    release(modules->names);
    release(modules->sym_maps);
    modules->names = NULL;
    modules->sym_maps = NULL;  
}

static void push_mod_array(struct ModArray *modules, struct Span name, struct SymMap map) 
{
    if (modules->cnt == modules->cap) {
        modules->cap *= 2;
        reallocate(modules->names, modules->cap * sizeof(struct Span));
        reallocate(modules->sym_maps, modules->cap * sizeof(struct SymMap));
    }
    modules->names[modules->cnt] = name;
    modules->sym_maps[modules->cnt] = map;
    modules->cnt++;
}

// TODO STACK TRACE
static void runtime_err(struct VM *vm, const char *msg) 
{
    printf("%s\n", msg);
    exit(1);
}

void init_vm(struct VM *vm)
{
    init_val_array(&vm->globals);
    init_mod_array(&vm->modules);
    vm->call_count = 0;
}

void release_vm(struct VM *vm) 
{
    release_val_array(&vm->globals);
    release_mod_array(&vm->modules);
}

// TODO check if exceeding max stack size
void run_vm(struct VM *vm, struct FnObj *fn) 
{
    Value *sp = vm->val_stack;

    struct CallFrame *frame = vm->call_stack;
    frame->fn = fn;
    frame->bp = sp;
    register u8 *ip = frame->fn->chunk.code;

    while(true) {
        u8 op = *ip;
        ip++;
        // printf("%s\n", opcode_str[op]);
        switch (op) {
        case OP_NIL: {
            sp[0] = NIL_VAL;
            sp++;
            break;
        }
        case OP_TRUE: {
            sp[0] = BOOL_VAL(true);
            sp++;
            break;
        }
        case OP_FALSE: {
            sp[0] = BOOL_VAL(false);
            sp++;
            break;
        }
        case OP_ADD: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = NUM_VAL(AS_NUM(lhs)+AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_SUB: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = NUM_VAL(AS_NUM(lhs)-AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_MUL: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = NUM_VAL(AS_NUM(lhs)*AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_DIV: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = NUM_VAL(AS_NUM(lhs)/AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_LT: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = BOOL_VAL(AS_NUM(lhs)<AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_LEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = BOOL_VAL(AS_NUM(lhs)<=AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_GT: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = BOOL_VAL(AS_NUM(lhs)>AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_GEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = BOOL_VAL(AS_NUM(lhs)>=AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(vm, "operands must be numbers");
            }
            break;
        }
        case OP_EQEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            sp[-2] = BOOL_VAL(val_eq(lhs, rhs));
            sp--;
            break;
        }
        case OP_NEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            sp[-2] = BOOL_VAL(!val_eq(lhs, rhs));
            sp--;
            break;
        }
        case OP_NEGATE: {
            Value val = sp[-1];
            if (IS_NUM(val)) {
                sp[-1] = NUM_VAL(-AS_NUM(val));
            } else {
                runtime_err(vm, "operand must be number");
            }
            break;
        }
        case OP_NOT: {
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                sp[-1] = BOOL_VAL(!AS_BOOL(val));
            } else {
                runtime_err(vm, "operand must be boolean");
            }
            break;
        }
        case OP_GET_CONST: {
            u16 idx = *ip++;
            sp[0] = frame->fn->chunk.constants.values[idx];
            sp++;
            break;
        }
        case OP_GET_LOCAL: {
            u8 idx = *ip++;
            sp[0] = frame->bp[idx];
            sp++;
            break;
        }
        case OP_SET_LOCAL: {
            u8 idx = *ip++;
            frame->bp[idx] = sp[-1];
            break;
        }
        case OP_GET_GLOBAL: {
            u8 idx = *ip++;
            sp[0] = vm->globals.values[idx];
            sp++;
            break;
        }
        case OP_SET_GLOBAL: {
            u8 idx = *ip++;
            vm->globals.values[idx] = sp[-1];
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
                runtime_err(vm, "operand must be boolean");
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
                runtime_err(vm, "operand must be boolean");
            }
            break;
        }
        case OP_CALL: {
            u8 arg_count = *ip++;
            Value val = sp[-1-arg_count];
            if (IS_FN(val)) {
                struct FnObj *fn = AS_FN(val);
                if (fn->arity != arg_count) {
                    printf("ARITY %d ARG COUNT %d\n", fn->arity, arg_count); // TODO REMOVE ME
                    runtime_err(vm, "incorrect number of arguments provided");
                }
                if (vm->call_count >= MAX_CALL_FRAMES)
                    runtime_err(vm, "stack overflow");
                frame->ip = ip; 
                
                vm->call_count++;
                frame++;
                frame->bp = sp-arg_count;
                frame->fn = fn;
                ip = fn->chunk.code;
            } else {
                runtime_err(vm, "attempt to call non-function");
            }
            break;
        }
        case OP_RETURN: {
            // copy return value
            frame->bp[-1] = sp[-1];
            frame--; 
            vm->call_count--;
            ip = frame->ip;
            if (vm->call_count == 0)
                return;
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
            break;
        }
        }
        // print_stack(vm, sp, frame->bp);
    }
}