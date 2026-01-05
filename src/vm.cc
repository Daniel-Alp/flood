#include "vm.h"
#include "chunk.h"
#include "dynarr.h"
#include "foreign.h"
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
        printf("[line %d] in %s\n", line, fn.name->str.chars());
    }
}

VM::VM() : call_stack(new CallFrame[MAX_CALL_FRAMES]), val_stack(new Value[MAX_STACK]), sp(val_stack), obj_list(nullptr)
{
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
    Value *sp = vm.val_stack;
    CallFrame *frame = vm.call_stack;
    // TODO we can probably take things out of frame to avoid going through pointer
    frame->closure = &script;
    // TODO some of these other things should probably be marked with register
    // also consider moving putting bp in its own var so I don't have to go through frame to reach bp each time
    frame->bp = sp;
    vm.call_cnt = 1;
    const u8 *ip = frame->closure->fn->chunk.code().raw(); // FIXME!!! frame->closure->fn.;
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
            HeapValObj *heap_val = alloc<HeapValObj>(vm, frame->bp[idx]);
            frame->bp[idx] = MK_OBJ(heap_val);
            break;
        }
        case OP_CLOSURE: {
            const u8 stack_captures = *ip++;
            const u8 parent_captures = *ip++;
            ClosureObj *closure = alloc<ClosureObj>(vm, AS_FN(sp[-1]), stack_captures + parent_captures);
            for (i32 i = 0; i < stack_captures; i++)
                closure->captures[i] = AS_HEAP_VAL(frame->bp[*ip++]);
            for (i32 i = 0; i < parent_captures; i++)
                closure->captures[stack_captures + i] = frame->closure->captures[*ip++];
            sp[-1] = MK_OBJ(closure);
            break;
        }
        case OP_GET_CONST: {
            const u16 idx = *ip++;
            sp[0] = frame->closure->fn->chunk.constants()[idx];
            sp++;
            break;
        }
        case OP_GET_LOCAL: {
            const u8 idx = *ip++;
            sp[0] = frame->bp[idx];
            sp++;
            break;
        }
        case OP_SET_LOCAL: {
            const u8 idx = *ip++;
            frame->bp[idx] = sp[-1];
            break;
        }
        case OP_GET_HEAPVAL: {
            const u8 idx = *ip++;
            sp[0] = AS_HEAP_VAL(frame->bp[idx])->val;
            sp++;
            break;
        }
        case OP_SET_HEAPVAL: {
            const u8 idx = *ip++;
            AS_HEAP_VAL(frame->bp[idx])->val = sp[-1];
            break;
        }
        case OP_GET_CAPTURED: {
            const u8 idx = *ip++;
            sp[0] = frame->closure->captures[idx]->val;
            sp++;
            break;
        }
        case OP_SET_CAPTURED: {
            const u8 idx = *ip++;
            frame->closure->captures[idx]->val = sp[-1];
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
            const u8 arg_count = *ip++;
            const Value val = sp[-1 - arg_count];
            ClosureObj *closure;
            if (IS_CLOSURE(val)) {
                closure = AS_CLOSURE(val);
            } else {
                runtime_err(ip, vm, "attempt to call non-callable");
                return INTERP_RUNTIME_ERR;
            }
            if (closure->fn->arity != arg_count) {
                runtime_err(ip, vm, "incorrect number of arguments provided");
                return INTERP_RUNTIME_ERR;
            }
            if (vm.call_cnt + 1 >= MAX_CALL_FRAMES) {
                runtime_err(ip, vm, "stack overflow");
                return INTERP_RUNTIME_ERR;
            }
            frame->ip = ip;
            vm.call_cnt++;
            frame++;
            frame->bp = sp - arg_count;
            frame->closure = closure;
            ip = closure->fn->chunk.code().raw();
            break;
        }
        case OP_RETURN: {
            vm.call_cnt--;
            if (vm.call_cnt == 0)
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
            frame->bp[-1] = sp[-1];
            sp = frame->bp;
            frame--;
            ip = frame->ip;
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
        vm.sp = sp;
        // printf("%s\n", opcode_str(OpCode(op)));
        // print_stack(vm, sp, frame->bp);
        // TODO don't run gc after every op, enable that only for testing
        // run it each iteration only if we define smth
        // FIXME add back!!!
        // collect_garbage(vm);
    }
}
