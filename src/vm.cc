#include "vm.h"
#include "chunk.h"
#include "dynarr.h"
#include "foreign.h"
#include "gc.h"
#include "object.h"
#include "value.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

static i32 get_opcode_line(Dynarr<i32> const &lines, const i32 tgt_opcode_idx)
{
    // see chunk.h
    i32 opcode_idx = 0;
    i32 i = 1;
    while (true) {
        opcode_idx += lines[i];
        if (opcode_idx >= tgt_opcode_idx)
            return lines[i - 1];
        i += 2;
    }
}

void runtime_err(const u8 *ip, VM &vm, const char *format, ...)
{
    // NOTE:
    // we put ip in a local variable for performance
    // this means we need to save the top frame's ip before we print the stack trace.
    // however, a foreign method can also call run_time err, in which case there is nothing to save.
    if (ip)
        vm.call_stack[vm.call_cnt - 1].ip = ip;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    for (i32 i = vm.call_cnt - 1; i >= 0; i--) {
        const FnObj &fn = *vm.call_stack[i].closure->fn;
        const i32 line = get_opcode_line(fn.chunk.lines(), vm.call_stack[i].ip - 1 - fn.chunk.code().raw());
        printf("[line %d] in %s\n", line, fn.name.chars());
    }
}

VM::VM() : call_stack(new CallFrame[MAX_CALL_FRAMES]), val_stack(new Value[MAX_STACK]), sp(val_stack), obj_list(nullptr)
{
    list_class = alloc<ClassObj>(*this, "List");
    define_list_methods(*this);
}

VM::~VM()
{
    delete[] call_stack;
    delete[] val_stack;
    while (obj_list) {
        Obj *obj = obj_list;
        obj_list = obj->next;
        delete obj;
    }
}

// TODO check if exceeding max stack size
InterpResult run_vm(VM &vm, ClosureObj &script)
{
    ClosureObj *cur_closure = &script;

    Value *sp = vm.val_stack;
    sp[0] = MK_OBJ(cur_closure);
    sp++;
    Value *bp = sp;

    CallFrame *frame = vm.call_stack;
    frame->closure = cur_closure;
    frame->bp = bp;
    vm.call_cnt = 1;

    const u8 *ip = cur_closure->fn->chunk.code().raw();

    while (true) {
        const u8 op = *ip;
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
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs) + AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_SUB: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs) - AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_MUL: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs) * AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_DIV: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(AS_NUM(lhs) / AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_FLOORDIV: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(floor(AS_NUM(lhs) / AS_NUM(rhs)));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_MOD: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_NUM(fmod(AS_NUM(lhs), AS_NUM(rhs)));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LT: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs) < AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LEQ: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs) <= AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_GT: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs) > AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_GEQ: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                sp[-2] = MK_BOOL(AS_NUM(lhs) >= AS_NUM(rhs));
                sp--;
            } else {
                runtime_err(ip, vm, "operands must be numbers");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_EQEQ: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            sp[-2] = MK_BOOL(val_eq(lhs, rhs));
            sp--;
            break;
        }
        case OP_NEQ: {
            const Value lhs = sp[-2];
            const Value rhs = sp[-1];
            sp[-2] = MK_BOOL(!val_eq(lhs, rhs));
            sp--;
            break;
        }
        case OP_NEGATE: {
            const Value val = sp[-1];
            if (IS_NUM(val)) {
                sp[-1] = MK_NUM(-AS_NUM(val));
            } else {
                runtime_err(ip, vm, "operand must be number");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_NOT: {
            const Value val = sp[-1];
            if (IS_BOOL(val)) {
                sp[-1] = MK_BOOL(!AS_BOOL(val));
            } else {
                runtime_err(ip, vm, "operand must be boolean");
                return INTERP_RUNTIME_ERR;
            }
            break;
        }
        case OP_LIST: {
            const u8 cnt = *ip++;
            sp -= cnt;
            Dynarr<Value> vals;
            for (Value *v = sp; v < sp + cnt; v++)
                vals.push(*v);
            ListObj *list = alloc<ListObj>(vm, move(vals));
            sp[0] = MK_OBJ(list);
            sp++;
            break;
        }
        case OP_HEAPVAL: {
            const u8 idx = *ip++;
            HeapValObj *heap_val = alloc<HeapValObj>(vm, bp[idx]);
            bp[idx] = MK_OBJ(heap_val);
            break;
        }
        case OP_CLOSURE: {
            const u8 stack_captures = *ip++;
            const u8 parent_captures = *ip++;
            ClosureObj *closure = alloc<ClosureObj>(vm, AS_FN(sp[-1]), stack_captures + parent_captures);
            for (i32 i = 0; i < stack_captures; i++)
                closure->captures[i] = AS_HEAP_VAL(bp[*ip++]);
            for (i32 i = 0; i < parent_captures; i++)
                closure->captures[stack_captures + i] = cur_closure->captures[*ip++];
            sp[-1] = MK_OBJ(closure);
            break;
        }
        case OP_GET_CONST: {
            const u16 idx = *ip++;
            sp[0] = cur_closure->fn->chunk.constants()[idx];
            sp++;
            break;
        }
        case OP_GET_LOCAL: {
            const u8 idx = *ip++;
            sp[0] = bp[idx];
            sp++;
            break;
        }
        case OP_SET_LOCAL: {
            const u8 idx = *ip++;
            bp[idx] = sp[-1];
            break;
        }
        case OP_GET_HEAPVAL: {
            const u8 idx = *ip++;
            sp[0] = AS_HEAP_VAL(bp[idx])->val;
            sp++;
            break;
        }
        case OP_SET_HEAPVAL: {
            const u8 idx = *ip++;
            AS_HEAP_VAL(bp[idx])->val = sp[-1];
            break;
        }
        case OP_GET_CAPTURED: {
            const u8 idx = *ip++;
            sp[0] = cur_closure->captures[idx]->val;
            sp++;
            break;
        }
        case OP_SET_CAPTURED: {
            const u8 idx = *ip++;
            cur_closure->captures[idx]->val = sp[-1];
            break;
        }
        case OP_GET_SUBSCR: {
            const Value container = sp[-2];
            const Value idx = sp[-1];
            if (IS_LIST(container)) {
                if (IS_NUM(idx)) {
                    if (AS_NUM(idx) >= 0 && AS_NUM(idx) < AS_LIST(container)->vals.len()) {
                        sp[-2] = AS_LIST(container)->vals[u32(AS_NUM(idx))];
                        sp--;
                    } else {
                        runtime_err(ip, vm, "index %d out of bounds for list of size %d", i32(AS_NUM(idx)),
                            AS_LIST(container)->vals.len());
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
            const Value container = sp[-3];
            const Value idx = sp[-2];
            const Value val = sp[-1];
            if (IS_LIST(container)) {
                if (IS_NUM(idx)) {
                    if (AS_NUM(idx) >= 0 && AS_NUM(idx) < AS_LIST(container)->vals.len()) {
                        AS_LIST(container)->vals[i32(AS_NUM(idx))] = val;
                        // TODO consider making assignment a statement rather than an expression
                        sp[-3] = sp[-1];
                        sp -= 2;
                    } else {
                        runtime_err(ip, vm, "index %d out of bounds for list of size %d", i32(AS_NUM(idx)),
                            i32(AS_LIST(container)->vals.len()));
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
            const u8 idx = *ip++;
            sp[0] = vm.globals[idx];
            sp++;
            break;
        }
        case OP_SET_GLOBAL: {
            const u8 idx = *ip++;
            vm.globals[idx] = sp[-1];
            break;
        }
        // TODO symbols and interning to optimize
        case OP_GET_FIELD: {
            const u8 idx = *ip++;
            StringObj *prop = AS_STRING(cur_closure->fn->chunk.constants()[idx]);
            Value val = sp[-1];
            if (IS_INSTANCE(val)) {
                InstanceObj *instance = AS_INSTANCE(val);
                Value *val = instance->fields.find(*prop);
                if (val != nullptr) {
                    sp[-1] = *val;
                    break;
                }
                runtime_err(
                    ip, vm, "`%s` instance does not have field `%s`", instance->klass->name.chars(), prop->str.chars());
                return INTERP_RUNTIME_ERR;
            }
            runtime_err(ip, vm, "cannot get field of non-user-instance");
            return INTERP_RUNTIME_ERR;
        }
        // TODO should distinguish between setting prop outside or within the instance
        case OP_SET_FIELD: {
            const u8 idx = *ip++;
            StringObj *prop = AS_STRING(cur_closure->fn->chunk.constants()[idx]);
            Value container = sp[-2];
            if (IS_INSTANCE(container)) {
                InstanceObj *instance = AS_INSTANCE(container);
                Value *val = instance->fields.find(*prop);
                if (val != nullptr) {
                    *val = sp[-1];
                } else {
                    // TODO check if field exists. do not want to create field from outside
                    // TODO make insert_val_table take hash to avoid recomputing it
                    instance->fields.insert(*prop, sp[-1]);
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
            const u8 idx = *ip++;
            StringObj *prop = AS_STRING(cur_closure->fn->chunk.constants()[idx]);
            Value val = sp[-1];
            ClassObj *klass = nullptr;
            if (IS_INSTANCE(val))
                klass = AS_INSTANCE(val)->klass;
            else if (IS_LIST(val))
                klass = vm.list_class;
            if (klass) {
                Value *fn = klass->methods.find(*prop);
                if (fn) {
                    if (AS_OBJ(*fn)->tag == OBJ_CLOSURE) {
                        // user-defined method, bind function to instance
                        auto method = alloc<MethodObj>(vm, AS_INSTANCE(val), AS_CLOSURE(*fn));
                        sp[-1] = MK_OBJ(method);
                    } else {
                        auto method = alloc<ForeignMethodObj>(vm, AS_OBJ(val), AS_FOREIGN_FN(*fn));
                        sp[-1] = MK_OBJ(method);
                    }
                    break;
                }
                runtime_err(ip, vm, "`%s` instance does not have method `%s`", klass->name.chars(), prop->str.chars());
                return INTERP_RUNTIME_ERR;
            }
            runtime_err(ip, vm, "cannot get method of non-instance");
            return INTERP_RUNTIME_ERR;
        }
        case OP_JUMP: {
            const u16 offset = (ip += 2, (ip[-2] << 8) | ip[-1]);
            ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            const u16 offset = (ip += 2, (ip[-2] << 8) | ip[-1]);
            const Value val = sp[-1];
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
            const u16 offset = (ip += 2, (ip[-2] << 8) | ip[-1]);
            const Value val = sp[-1];
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
            const u8 arg_cnt = *ip++;
            u8 param_cnt = arg_cnt;
            const Value val = sp[-arg_cnt - 1];
            ClosureObj *closure;
            if (IS_CLOSURE(val)) {
                closure = AS_CLOSURE(val);
            } else if (IS_CLASS(val)) {
                ClassObj *klass = AS_CLASS(val);
                InstanceObj *instance = alloc<InstanceObj>(vm, klass);
                closure = AS_CLOSURE(*instance->klass->methods.find(*alloc<StringObj>(vm, "init")));
                sp[0] = MK_OBJ(instance);
                sp++;
                param_cnt++;
            } else if (IS_METHOD(val)) {
                MethodObj *method = AS_METHOD(val);
                closure = method->closure;
                sp[0] = MK_OBJ(method->self);
                sp++;
                param_cnt++;
            } else if (IS_FOREIGN_METHOD(val)) {
                // TODO all of this is sketchy
                ForeignMethodObj *f_method = AS_FOREIGN_METHOD(val);
                if (arg_cnt+1 != f_method->fn->arity) {
                    runtime_err(ip, vm, "incorrect number of arguments provided");
                    return INTERP_RUNTIME_ERR;
                }
                sp[0] = MK_OBJ(f_method->self);
                sp++;
                vm.sp = sp;
                if (!f_method->fn->code(vm)) {
                    return INTERP_RUNTIME_ERR;
                }
                sp -= arg_cnt+1; // TODO this is sketchy 
                break;
            } else {
                runtime_err(ip, vm, "attempt to call non-callable");
                return INTERP_RUNTIME_ERR;
            }
            if (closure->fn->arity != arg_cnt) {
                runtime_err(ip, vm, "incorrect number of arguments provided");
                return INTERP_RUNTIME_ERR;
            }
            if (vm.call_cnt + 1 >= MAX_CALL_FRAMES) {
                runtime_err(ip, vm, "stack overflow");
                return INTERP_RUNTIME_ERR;
            }
            frame->ip = ip;
            frame->bp = bp;
            frame++;

            cur_closure = closure;
            frame->closure = cur_closure;
            bp = sp - param_cnt;
            ip = cur_closure->fn->chunk.code().raw();
            vm.call_cnt++;
            break;
        }
        case OP_RETURN: {
            vm.call_cnt--;
            if (vm.call_cnt == 0)
                return INTERP_OK;

            bp[-1] = sp[-1];
            sp = bp;

            frame--;
            ip = frame->ip;
            bp = frame->bp;
            cur_closure = frame->closure;
            break;
        }
        case OP_POP: {
            sp--;
            break;
        }
        case OP_POP_N: {
            const u8 n = *ip++;
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
        // printf("%s\n", cur_closure->fn->name.chars());
        // printf("%s\n", vm.call_stack[vm.call_cnt-1].closure->fn->name.chars());
        // printf("%s\n", opcode_str(OpCode(op)));
        // printf("functions on stack\n");
        // for (i32 i = 0; i < vm.call_cnt; i++) {
        //     printf("%s\n", vm.call_stack[i].closure->fn->name.chars());
        // }
        // print_stack(vm, sp, bp);
        // TODO don't run gc after every op, enable that only for testing
        // run it each iteration only if we define smth
        // collect_garbage(vm);
    }
}
