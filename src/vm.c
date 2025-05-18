#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "memory.h"
#include "object.h"

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
    vm->sp = vm->val_stack;
}

void release_vm(struct VM *vm) 
{
    release_val_array(&vm->globals);
    release_mod_array(&vm->modules);
}

// TEMP remove
void print_stack(struct VM *vm, Value *sp, Value *bp) {
    printf("stack contents\n");
    for (i32 i = 0; i < 12; i++) {
        if (vm->val_stack + i == sp)
            printf("sp > ");
        else if (vm->val_stack + i == bp)
            printf("bp > ");
        else    
            printf("     ");
        Value val = vm->val_stack[i];
        switch(val.tag) {
        case VAL_NUM:  printf("%.4f\n", AS_NUM(val)); break;
        case VAL_BOOL: printf("%s\n", AS_BOOL(val) ? "true" : "false"); break;
        case VAL_NIL:  printf("null\n"); break;
        case VAL_OBJ: 
            if (IS_FN(val)) {
                struct Span span = AS_FN(val)->span;
                printf("<function %.*s>\n", span.length, span.start);
            }
            break;
        }
    }
    printf("\n");
}

// TEMP remove
void print_op(u8 op) {
    switch(op){
    case OP_NIL:           printf("OP_NIL\n"); break;
    case OP_TRUE:          printf("OP_TRUE\n"); break;
    case OP_FALSE:         printf("OP_FALSE\n"); break;
    case OP_ADD:           printf("OP_ADD\n"); break;
    case OP_SUB:           printf("OP_SUB\n"); break;
    case OP_MUL:           printf("OP_MUL\n"); break;        
    case OP_DIV:           printf("OP_DIV\n"); break;  
    case OP_LT:            printf("OP_LT\n"); break;  
    case OP_LEQ:           printf("OP_LEQ\n"); break;  
    case OP_GT:            printf("OP_GT\n"); break;  
    case OP_GEQ:           printf("OP_GEQ\n"); break;  
    case OP_EQEQ:          printf("OP_EQ_EQ\n"); break;
    case OP_NEQ:           printf("OP_NEQ\n"); break;
    case OP_NEGATE:        printf("OP_NEGATE\n"); break;
    case OP_NOT:           printf("OP_NOT\n"); break;    
    case OP_GET_CONST:     printf("OP_GET_CONST\n"); break;
    case OP_GET_LOCAL:     printf("OP_GET_LOCAL\n"); break;
    case OP_SET_LOCAL:     printf("OP_SET_LOCALd\n"); break;
    case OP_GET_GLOBAL:    printf("OP_GET_GLOBAL\n"); break;
    case OP_SET_GLOBAL:    printf("OP_SET_GLOBAL\n"); break;
    case OP_JUMP_IF_FALSE: printf("OP_JUMP_IF_FALSE\n"); break;
    case OP_JUMP_IF_TRUE:  printf("OP_JUMP_IF_TRUE\n"); break;
    case OP_JUMP:          printf("OP_JUMP\n"); break;       
    case OP_CALL:          printf("OP_CALL\n"); break;
    case OP_RETURN:        printf("OP_RETURN\n"); break;
    case OP_POP:           printf("OP_POP\n"); break;
    case OP_POP_N:         printf("OP_POP_N\n"); break;
    case OP_PRINT:         printf("OP_PRINT\n"); break;
    }
}

// TODO check if exceeding max stack size
void run_vm(struct VM *vm, struct FnObj *fn) 
{
    Value *sp = vm->sp;

    struct CallFrame *frame = vm->call_stack;
    frame->fn = fn;
    frame->bp = vm->sp;
    register u8 *ip = frame->fn->chunk.code;

    while(true) {
        u8 op = *ip;
        ip++;
        // print_op(op);
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
            break;
        }
        case OP_NEQ: {
            Value lhs = sp[-2];
            Value rhs = sp[-1];
            sp[-2] = BOOL_VAL(!val_eq(lhs, rhs));
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
                if (fn->arity != arg_count)
                    runtime_err(vm, "incorrect number of arguments provided");
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
            Value val = sp[-1];
            switch(val.tag) {
            case VAL_NUM:  printf("%.4f\n", AS_NUM(val)); break;
            case VAL_BOOL: printf("%s\n", AS_BOOL(val) ? "true" : "false"); break;
            case VAL_NIL:  printf("null\n"); break;
            case VAL_OBJ: 
                if (IS_FN(val)) {
                    struct Span span = AS_FN(val)->span;
                    printf("<function %.*s>\n", span.length, span.start);
                }
                break;
            }
            sp--;
            break;
        }
        }
        // print_stack(vm, sp, frame->bp);
    }
}