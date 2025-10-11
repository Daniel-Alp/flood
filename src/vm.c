#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "object.h"
#include "debug.h"
#include "foreign.h"

static i32 get_opcode_line(i32 *lines, i32 tgt_opcode_idx)
{
    // see chunk.h
    i32 opcode_idx = 0;
    i32 i = 1;
    while (true) {
        opcode_idx += lines[i];
        if (opcode_idx >= tgt_opcode_idx)
            return lines[i-1];
        i += 2;
    }
}

void runtime_err(u8 *ip, struct VM *vm, const char *format, ...) 
{
    // NOTE: 
    // we put ip in a local variable for performance
    // this means we need to save the top frame's ip before we print the stack trace.
    // however, a foreign method can also call run_time err, in which case there is nothing to save.
    if (ip)
        vm->call_stack[vm->call_cnt-1].ip = ip;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    for (i32 i = vm->call_cnt-1; i >= 1; i--) {
        struct CallFrame frame = vm->call_stack[i];
        i32 line = get_opcode_line(frame.closure->fn->chunk.lines, frame.ip-1 - frame.closure->fn->chunk.code);
        printf("[line %d] in %s\n", line, frame.closure->fn->name->chars);
    }
}

void init_vm(struct VM *vm)
{
    vm->call_stack = allocate(sizeof(struct CallFrame) * MAX_CALL_FRAMES);
    vm->sp = vm->val_stack;
    vm->obj_list = NULL;
    init_val_array(&vm->globals);
    vm->gray_cnt = 0;
    vm->gray_cap = 8;
    vm->gray = allocate(vm->gray_cap * sizeof(struct Obj*));

    vm->list_class = (struct ClassObj*)alloc_vm_obj(vm, sizeof(struct ClassObj));
    vm->string_class = (struct ClassObj*)alloc_vm_obj(vm, sizeof(struct ClassObj));
    vm->class_class = (struct ClassObj*)alloc_vm_obj(vm, sizeof(struct ClassObj));

    init_class_obj(vm->list_class, string_from_c_str(vm, "List"), vm);
    init_class_obj(vm->string_class, string_from_c_str(vm, "String"), vm);
    init_class_obj(vm->class_class, string_from_c_str(vm, "Class"), vm);

    define_list_methods(vm);
}

void release_vm(struct VM *vm) 
{
    release(vm->call_stack);
    vm->call_stack = NULL;
    vm->sp = NULL;
    while (vm->obj_list) {
        struct Obj *obj = vm->obj_list;
        vm->obj_list = vm->obj_list->next;
        release_obj(obj);
        release(obj);
    }
    vm->obj_list = NULL;
    release_val_array(&vm->globals);
    vm->gray_cnt = 0;
    vm->gray_cap = 0;
    release(vm->gray);
    vm->gray = NULL;
}

// TODO check if exceeding max stack size
enum InterpResult run_vm(struct VM *vm, struct ClosureObj *script) 
{
    Value *sp = vm->val_stack;

    struct CallFrame *frame = vm->call_stack;
    // TODO we can probably take things out of frame to avoid going through pointer
    frame->closure = script;
    // TODO some of these other things should probably be marked with register
    // also consider moving putting bp in its own var so I don't have to go through frame to reach bp each time
    frame->bp = sp;
    vm->call_cnt = 1;
    register u8 *ip = frame->closure->fn->chunk.code;
    while(true) {
        u8 op = *ip;
        ip++;
        switch (op) {
        case OP_NULL: {
            sp[0] = MK_NULL;
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operands must be numbers");
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
                runtime_err(ip, vm, "operand must be number");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_NOT: {
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                sp[-1] = MK_BOOL(!AS_BOOL(val));
            } else {
                runtime_err(ip, vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LIST: {
            u8 cnt = *ip++;
            sp -= cnt;
            Value *vals = sp;
            struct ListObj *list = (struct ListObj*)alloc_vm_obj(vm, sizeof(struct ListObj));
            init_list_obj(list, vals, cnt, vm);
            sp[0] = MK_OBJ((struct Obj*)list);
            sp++;
            break;
        }
        case OP_CLASS: {
            u8 idx = *ip++;
            struct StringObj *name = AS_STRING(frame->closure->fn->chunk.constants.vals[idx]);
            struct ClassObj *class = (struct ClassObj*)alloc_vm_obj(vm, sizeof(struct ClassObj));
            init_class_obj(class, name, vm);
            sp[0] = MK_OBJ((struct Obj*)class);
            sp++;
            break;
        }
        // TODO will need something for allowing user-defined foreign methods later
        case OP_METHOD: {
            struct ClassObj *class = AS_CLASS(sp[-2]);
            struct StringObj *name = AS_CLOSURE(sp[-1])->fn->name;
            // TODO optimize, avoid recomputing hash each time
            insert_val_table(&class->methods, name->chars, name->len, sp[-1]);
            sp--;
            break;
        }
        case OP_HEAPVAL: {
            u8 idx = *ip++;
            struct HeapValObj *heap_val = (struct HeapValObj*)alloc_vm_obj(vm, sizeof(struct HeapValObj));
            init_heap_val_obj(heap_val, frame->bp[idx]);
            frame->bp[idx] = MK_OBJ((struct Obj*)heap_val);
            break;
        }
        case OP_CLOSURE: {
            u8 stack_captures = *ip++;
            u8 parent_captures = *ip++;
            struct ClosureObj *closure = (struct ClosureObj*)alloc_vm_obj(vm, sizeof(struct ClosureObj));
            init_closure_obj(closure, AS_FN(sp[-1]), stack_captures + parent_captures);
            for (i32 i = 0; i < stack_captures; i++)
                closure->captures[i] = AS_HEAP_VAL(frame->bp[*ip++]);
            for (i32 i = 0; i < parent_captures; i++)
                closure->captures[stack_captures+i] = frame->closure->captures[*ip++];
            sp[-1] = MK_OBJ((struct Obj*)closure);
            break;
        }
        case OP_GET_CONST: {
            u16 idx = *ip++;
            sp[0] = frame->closure->fn->chunk.constants.vals[idx];
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
        case OP_GET_HEAPVAL: {
            u8 idx = *ip++;
            sp[0] = AS_HEAP_VAL(frame->bp[idx])->val;
            sp++;
            break; 
        }
        case OP_SET_HEAPVAL: {
            u8 idx = *ip++;
            AS_HEAP_VAL(frame->bp[idx])->val = sp[-1];
            break;
        }
        case OP_GET_CAPTURED: {
            u8 idx = *ip++;
            sp[0] = frame->closure->captures[idx]->val;
            sp++;
            break;
        }
        case OP_SET_CAPTURED: {
            u8 idx = *ip++;
            frame->closure->captures[idx]->val = sp[-1];
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
                        runtime_err(ip, 
                            vm, 
                            "index %d out of bounds for list of size %d", 
                            (i32)AS_NUM(idx), 
                            (i32)AS_LIST(container)->cnt
                        );                       
                        return INTERP_RUNTIME_ERR;
                    }
                } else {
                    runtime_err(ip, vm, "list index must be number");
                    return INTERP_RUNTIME_ERR;
                }
            } else {
                runtime_err(ip, vm, "object is not subscriptable");
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
                        AS_LIST(container)->vals[(i32)AS_NUM(idx)] = val;
                        // TODO consider making assignment a statement rather than an expression
                        sp[-3] = sp[-1];
                        sp -= 2;
                    } else {
                        runtime_err(ip, 
                            vm, 
                            "index %d out of bounds for list of size %d", 
                            (i32)AS_NUM(idx), 
                            (i32)AS_LIST(container)->cnt
                        );
                        return INTERP_RUNTIME_ERR;
                    }
                } else {
                    runtime_err(ip, vm, "list index must be number");
                    return INTERP_RUNTIME_ERR;
                }
            } else {
                runtime_err(ip, vm, "object is not subscriptable");
                return INTERP_RUNTIME_ERR;
            }
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
        // TODO string interning (and later symbols) to optimize method lookup
        case OP_GET_FIELD: {
            u8 idx = *ip++;
            struct StringObj *prop = AS_STRING(frame->closure->fn->chunk.constants.vals[idx]);
            Value val = sp[-1];
        
            // attempt to lookup field
            if (IS_INSTANCE(val)) {
                struct InstanceObj *instance = AS_INSTANCE(val);

                struct ValTableEntry *entry = get_val_table_slot(
                    instance->fields.entries, 
                    instance->fields.cap,
                    prop->hash,
                    prop->len,
                    prop->chars
                );
                if (entry->chars != NULL) {
                    sp[-1] = entry->val;
                    break;
                }
                runtime_err(ip, vm, "`%s` instance does not have field `%s`", instance->base.class->name->chars, prop->chars);
                return INTERP_RUNTIME_ERR;
            } 
            runtime_err(ip, vm, "cannot get field of non-user-instance");
            return INTERP_RUNTIME_ERR;
        }
        // TODO should distinguish between setting prop outside or within the instance
        case OP_SET_FIELD: {
            u8 idx = *ip++;
            struct StringObj *prop = AS_STRING(frame->closure->fn->chunk.constants.vals[idx]);
            Value container = sp[-2];
            Value val = sp[-1];

            if (IS_INSTANCE(container)) {
                struct InstanceObj *instance = AS_INSTANCE(container);
                struct ValTableEntry *entry = get_val_table_slot(
                    instance->fields.entries, 
                    instance->fields.cap,
                    prop->hash,
                    prop->len,
                    prop->chars
                );
                if (entry->chars != NULL) {
                    entry->val = val;
                } else {
                    // TODO check if field exists. do not want to create field from outside
                    // TODO make insert_val_table take hash to avoid recomputing it
                    insert_val_table(&instance->fields, prop->chars, prop->len, val);
                }
                sp[-2] = sp[-1];
                sp--;
                break;
            }
            runtime_err(ip, vm, "cannot set field of non-user-instance");
            return INTERP_RUNTIME_ERR;
        }
        // TODO implement OP_INVOKE optimization
        case OP_GET_METHOD: {
            u8 idx = *ip++;
            struct StringObj *prop = AS_STRING(frame->closure->fn->chunk.constants.vals[idx]);
            Value val = sp[-1];
            if(IS_OBJ(val)) {
                struct ClassObj *class = AS_OBJ(val)->class;
                struct ValTableEntry *entry = get_val_table_slot(
                    class->methods.entries,
                    class->methods.cap,
                    prop->hash,
                    prop->len,
                    prop->chars
                );
                if (entry->chars != NULL) {
                    if (AS_OBJ(entry->val)->tag == OBJ_CLOSURE) { 
                         // user-defined method, bind function to instance
                        struct MethodObj *method = (struct MethodObj*)alloc_vm_obj(vm, sizeof(struct MethodObj));
                        init_method_obj(method, AS_CLOSURE(entry->val), AS_INSTANCE(val));
                        sp[-1] = MK_OBJ((struct Obj*)method); 
                    } else { 
                        // foreign method, bind function to instance
                        struct ForeignMethodObj *f_method = (struct ForeignMethodObj*)alloc_vm_obj(vm, sizeof(struct ForeignMethodObj));
                        init_foreign_method_obj(f_method, AS_FOREIGN_FN(entry->val), (struct Obj*)AS_INSTANCE(val));
                        sp[-1] = MK_OBJ((struct Obj*)f_method);
                    }
                    break;
                }
                runtime_err(ip, vm, "`%s` instance does not have method `%s`", class->name->chars, prop->chars);
                return INTERP_RUNTIME_ERR;
            }
            runtime_err(ip, vm, "cannot get method of non-instance");
            return INTERP_RUNTIME_ERR;
        }
        case OP_JUMP: {
            u16 offset = (ip += 2, (ip[-2] << 8) | ip[-1]);
            ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            u16 offset = (ip += 2, (ip[-2] << 8) | ip[-1]);
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                if (!AS_BOOL(val))
                    ip += offset;
            } else {
                runtime_err(ip, vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_JUMP_IF_TRUE: {
            u16 offset = (ip += 2, (ip[-2] << 8) | ip[-1]);
            Value val = sp[-1];
            if (IS_BOOL(val)) {
                if (AS_BOOL(val))
                    ip += offset;
            } else {
                runtime_err(ip, vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_CALL: {
            u8 arg_count = *ip++;
            Value val = sp[-1-arg_count];
            if (IS_FOREIGN_METHOD(val)) {
                struct ForeignMethodObj *f_method = AS_FOREIGN_METHOD(val);
                sp[0] = MK_OBJ((struct Obj*)f_method->self);
                sp++;
                arg_count++;
                if (f_method->fn->arity != arg_count) {
                    runtime_err(ip, vm, "incorrect number of arguments provided");
                    return INTERP_RUNTIME_ERR;
                }
                if (vm->call_cnt+1 >= MAX_CALL_FRAMES) {
                    runtime_err(ip, vm, "stack overflow");
                    return INTERP_RUNTIME_ERR;
                }
                frame->ip = ip;
                vm->sp = sp;
                if (!f_method->fn->code(vm)) {
                    return INTERP_RUNTIME_ERR;
                }
                sp -= arg_count;
                break; // breaks out of case
            } 
            struct ClosureObj *closure;
            if (IS_CLASS(val)) {
                struct ClassObj *class = AS_CLASS(val);
                struct InstanceObj *instance = (struct InstanceObj*)alloc_vm_obj(vm, sizeof(struct InstanceObj));
                init_instance_obj(instance, class);
                struct ValTableEntry *entry = get_val_table_slot(
                    class->methods.entries,
                    class->methods.cap,
                    hash_string("init", 4),
                    4,
                    "init"
                );
                struct MethodObj *init = (struct MethodObj*)alloc_vm_obj(vm, sizeof(struct MethodObj));
                init_method_obj(init, AS_CLOSURE(entry->val), instance);
                // replace <class obj> with <method init>
                //      <class obj>
                //      <arg>
                //      ...
                //      <arg>
                sp[-1-arg_count] = MK_OBJ((struct Obj*)init);
                closure = init->closure;
                sp[0] = MK_OBJ((struct Obj*)instance);
                sp++;
                arg_count++;
            } else if (IS_METHOD(val)) {
                struct MethodObj *method = AS_METHOD(val);
                closure = method->closure;
                sp[0] = MK_OBJ((struct Obj*)method->self);
                sp++;
                arg_count++;
            } else if (IS_CLOSURE(val)) {
                closure = AS_CLOSURE(val);
            } else {
                runtime_err(ip, vm, "attempt to call non-callable");
                return INTERP_RUNTIME_ERR;
            }
            if (closure->fn->arity != arg_count) {
                runtime_err(ip, vm, "incorrect number of arguments provided");
                return INTERP_RUNTIME_ERR;
            }
            if (vm->call_cnt+1 >= MAX_CALL_FRAMES) {
                runtime_err(ip, vm, "stack overflow");
                return INTERP_RUNTIME_ERR;
            }
            frame->ip = ip;
            vm->call_cnt++;
            frame++;
            frame->bp = sp-1-arg_count;
            frame->closure = closure;
            ip = closure->fn->chunk.code;
            break;
        }
        case OP_RETURN: {
            vm->call_cnt--;
            if (vm->call_cnt == 0)
                return INTERP_OK;
            // NOTE: 
            // when a function returns the stack looks like this
            // bp-> <function foo> 
            //      <arg>
            //      ...
            //      <arg>
            //      <local>
            //      ...
            //      <local> <-return value
            // sp-> 
            frame->bp[0] = sp[-1];
            sp = frame->bp + 1;   
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
        // TODO don't run gc after every op, enable that only for testing
        // run it each iteration only if we define smth

        collect_garbage(vm);
    }
}
