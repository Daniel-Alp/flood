#include <stdlib.h>
#include <stdio.h>
#include "vm.h"
#define MAX_STACK 1024

void run(struct Chunk *chunk) {
    // TODO error if exceed max stack size
    Value stack[MAX_STACK];
    Value *stack_top = &stack[0];
    u8 *ip = chunk->code;
    while(true) {
        u8 op = *ip;
        ip++;
        switch(op) {
        case OP_DUMMY: {
            stack_top++;
            break;
        }
        case OP_TRUE: {
            stack_top[0] = BOOL_VAL(true);
            stack_top++;
            break;
        }
        case OP_FALSE: {
            stack_top[0] = BOOL_VAL(false);
            stack_top++;
            break;
        }
        case OP_ADD: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = NUM_VAL(AS_NUM(lhs)+AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_SUB: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = NUM_VAL(AS_NUM(lhs)-AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_MUL: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = NUM_VAL(AS_NUM(lhs)*AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_DIV: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = NUM_VAL(AS_NUM(lhs)/AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_LT: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = BOOL_VAL(AS_NUM(lhs)<AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_LEQ: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = BOOL_VAL(AS_NUM(lhs)<=AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_GT: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = BOOL_VAL(AS_NUM(lhs)>AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_GEQ: {
            Value lhs = stack_top[-2];
            Value rhs = stack_top[-1];
            if (IS_NUM(lhs) && IS_NUM(rhs)) {
                stack_top[-2] = BOOL_VAL(AS_NUM(lhs)>=AS_NUM(rhs));
                stack_top--;
            } else {
                printf("Operands must be numbers\n");
                exit(1);
            }
            break;
        }
        case OP_EQ_EQ: {
            stack_top[-2] = BOOL_VAL(values_equal(stack_top[-2], stack_top[-1]));
            stack_top--;
            break;
        }
        case OP_NEQ: {
            stack_top[-2] = BOOL_VAL(!values_equal(stack_top[-2], stack_top[-1]));
            stack_top--;
            break;
        }
        case OP_NEGATE: {
            Value val = stack_top[-1];
            if (IS_NUM(val)) {
                stack_top[-1] = NUM_VAL(-AS_NUM(val));
            } else {
                printf("Operand must be number\n");
                exit(1);
            }
            break;
        }
        case OP_NOT: {
            Value val = stack_top[-1];
            if (IS_BOOL(val)) {
                stack_top[-1] = BOOL_VAL(!AS_BOOL(val));
            } else {
                printf("Operand must be boolean\n");
                exit(1);
            }
            break;
        }
        case OP_GET_CONST: {
            u8 idx = *ip++;
            stack_top[0] = chunk->constants.values[idx];
            stack_top++;
            break;
        }
        case OP_GET_LOCAL: {
            u8 idx = *ip++;
            stack_top[0] = stack[idx];
            stack_top++;
            break;
        }
        case OP_SET_LOCAL: {
            u8 idx = *ip++;
            stack[idx] = stack_top[-1];
            break;
        }
        case OP_JUMP: {
            ip += ((*ip++) << 8) + (*ip++);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            u16 offset = ((*ip++) << 8) + (*ip++);
            Value val = stack_top[-1];
            if (IS_BOOL(val)) {
                if (!AS_BOOL(val))
                    ip += offset;
            } else {
                printf("Operand must be boolean\n");
                exit(1);
            }
            break;
        }
        case OP_JUMP_IF_TRUE: {
            u16 offset = ((*ip++) << 8) + (*ip++);
            Value val = stack_top[-1];
            if (IS_BOOL(val)) {
                if (AS_BOOL(val))
                    ip += offset;
            } else {
                printf("Operand must be boolean\n");
                exit(1);
            }
            break;
        }
        case OP_RETURN: 
            return;
        case OP_POP: {
            stack_top--;
            break;
        }
        case OP_PRINT: {
            Value val = stack_top[-1];
            switch(val.tag) {
                case VAL_NUM:  printf("%.4f\n", AS_NUM(val)); break;
                case VAL_BOOL: printf("%s\n", AS_BOOL(val) ? "true" : "false"); break;
                case VAL_NIL:  printf("null\n"); break;
            }
            stack_top--;
            break;
        }
        }
    }
}