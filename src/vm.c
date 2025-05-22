#include <stdlib.h>
#include <stdio.h>
#include "vm.h"
#include "memory.h"
#include "object.h"
#include "debug.h"

static void init_file_array(struct FileArray *files) 
{
    files->cnt = 0;
    files->cap = 8;
    files->paths = allocate(files->cap * sizeof(char*));
    files->sym_maps = allocate(files->cap * sizeof(struct SymMap));
}

static void release_file_array(struct FileArray *files) 
{
    for (i32 i = 0; i < files->cnt; i++) {
        // paths were malloc'd by realpath, so we call free instead of release
        // because we want every call to allocate to pair with a call to release
        free(files->paths[i]);
        release_symbol_map(&files->sym_maps[i]);
    }
    files->cnt = 0;
    files->cap = 0;
    release(files->paths);
    release(files->sym_maps);
    files->paths = NULL;
    files->sym_maps = NULL;  
}

void push_file_array(struct FileArray *files, const char *name, struct SymMap map) 
{
    if (files->cnt == files->cap) {
        files->cap *= 2;
        reallocate(files->paths, files->cap * sizeof(char*));
        reallocate(files->sym_maps, files->cap * sizeof(struct SymMap));
    }
    files->paths[files->cnt] = name;
    files->sym_maps[files->cnt] = map;
    files->cnt++;
}

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

// TODO don't exit instead return err
static void runtime_err(struct VM *vm, const char *msg) 
{
    printf("%s\n", msg);
    for (i32 i = vm->call_count-1; i >= 1; i--) {
        struct CallFrame frame = vm->call_stack[i];
        u32 line = get_opcode_line(frame.fn->chunk.lines, frame.ip-1 - frame.fn->chunk.code);
        printf("[line %d] in %s\n", line, frame.fn->name);
    }
}

// TODO bench and rework
struct Obj *alloc_vm_obj(struct VM *vm, u64 size)
{
    struct Obj *obj = allocate(size);
    obj->next = vm->obj_list;
    vm->obj_list = obj;
    return obj;
}

void release_vm_obj(struct VM *vm)
{
    while (vm->obj_list) {
        struct Obj *obj = vm->obj_list;
        switch(obj->tag) {
        case OBJ_FN: {
            struct FnObj *fn = (struct FnObj*)obj;
            release_chunk(&fn->chunk);
            release(fn->name);
            fn->name = NULL;
            break;
        }
        }
        vm->obj_list = vm->obj_list->next;
        release(obj);
    }
}

void init_vm(struct VM *vm, const char *source_dir)
{
    init_val_array(&vm->globals);
    init_file_array(&vm->files);
    init_arena(&vm->arena);
    vm->obj_list = NULL;
    vm->source_dir = source_dir;
}

void release_vm(struct VM *vm) 
{
    release_val_array(&vm->globals);
    release_file_array(&vm->files);
    release_arena(&vm->arena);
    release_vm_obj(vm);
}

// TODO check if exceeding max stack size
enum InterpResult run_vm(struct VM *vm, struct FnObj *fn) 
{
    Value *sp = vm->val_stack;

    struct CallFrame *frame = vm->call_stack;
    frame->fn = fn;
    frame->bp = sp;
    vm->call_count = 1;
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
                sp[-2] = NUM_VAL(AS_NUM(lhs)-AS_NUM(rhs));
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
                sp[-2] = NUM_VAL(AS_NUM(lhs)*AS_NUM(rhs));
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
                sp[-2] = NUM_VAL(AS_NUM(lhs)/AS_NUM(rhs));
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
                sp[-2] = BOOL_VAL(AS_NUM(lhs)<AS_NUM(rhs));
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
                sp[-2] = BOOL_VAL(AS_NUM(lhs)<=AS_NUM(rhs));
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
                sp[-2] = BOOL_VAL(AS_NUM(lhs)>AS_NUM(rhs));
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
                sp[-2] = BOOL_VAL(AS_NUM(lhs)>=AS_NUM(rhs));
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
                frame->ip = ip;
                runtime_err(vm, "operand must be number");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_NOT: {
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                sp[-1] = BOOL_VAL(!AS_BOOL(val));
            } else {
                frame->ip = ip;
                runtime_err(vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
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
                if (vm->call_count+1 >= MAX_CALL_FRAMES) {
                    frame->ip = ip;
                    runtime_err(vm, "stack overflow");
                    return INTERP_RUNTIME_ERR;
                }
                frame->ip = ip; 
                
                vm->call_count++;
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
            vm->call_count--;
            if (vm->call_count == 0)
                return INTERP_OK;
            // copy return value
            frame->bp[-1] = sp[-1];
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
            break;
        }
        }
        // print_stack(vm, sp, frame->bp);
    }
}