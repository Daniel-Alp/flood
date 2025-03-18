#include <stdio.h>
#include "vm.h"

#define BINARY_OP(vm, op) \
    do { \
      Value b = pop(vm); \
      Value a = pop(vm); \
      push(vm, a op b); \
    } while (false)

static Value pop(struct VM *vm) {
    vm->stack_top--;
    return *vm->stack_top;
}

static void push(struct VM *vm, Value val) {
    vm->stack_top[0] = val;
    vm->stack_top++;
}

void init_vm(struct VM *vm, struct Chunk chunk) {
    vm->chunk = chunk;
    vm->ip = chunk.code;
    vm->stack_top = vm->value_stack;
}

void run(struct VM *vm) {
    while (true) {
        switch (*(vm->ip)) {
            case OP_ADD: 
                BINARY_OP(vm, +);
                break;
            case OP_SUB:
                BINARY_OP(vm, -);
                break;
            case OP_MUL:
                BINARY_OP(vm, *);
                break;
            case OP_DIV:
                BINARY_OP(vm, /);
                break;
            case OP_NEGATE:
                push(vm, -pop(vm));                
                break;
            case OP_CONST:
                vm->ip++;
                u8 idx = *(vm->ip);
                push(vm, vm->chunk.constants.arr[idx]);
                break;
            case OP_RETURN:
                printf("%f\n", pop(vm));
                return;
        }
        vm->ip++;
    }
}