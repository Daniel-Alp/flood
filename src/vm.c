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
            stack_top[0] = 1;
            stack_top++;
            break;
        }
        case OP_FALSE: {
            stack_top[0] = 0;
            stack_top++;
            break;
        }
        case OP_ADD: {
            stack_top[-2] = stack_top[-2] + stack_top[-1];
            stack_top--;
            break;
        }
        case OP_SUB: {
            stack_top[-2] = stack_top[-2] - stack_top[-1];
            stack_top--;
            break;
        }
        case OP_MUL: {
            stack_top[-2] = stack_top[-2] * stack_top[-1];
            stack_top--;
            break;
        }
        case OP_DIV: {
            stack_top[-2] = stack_top[-2] / stack_top[-1];
            stack_top--;
            break;
        }
        case OP_LT: {
            stack_top[-2] = stack_top[-2] < stack_top[-1];
            stack_top--;
            break;
        }
        case OP_LEQ: {
            stack_top[-2] = stack_top[-2] <= stack_top[-1];
            stack_top--;
            break;
        }
        case OP_GT: {
            stack_top[-2] = stack_top[-2] > stack_top[-1];
            stack_top--;
            break;
        }
        case OP_GEQ: {
            stack_top[-2] = stack_top[-2] >= stack_top[-1];
            stack_top--;
            break;
        }
        case OP_EQ_EQ: {
            stack_top[-2] = stack_top[-2] == stack_top[-1];
            stack_top--;
            break;
        }
        case OP_NEQ: {
            stack_top[-2] = stack_top[-2] != stack_top[-1];
            stack_top--;
            break;
        }
        case OP_NEGATE: {
            stack_top[-1] = -stack_top[-1];
            break;
        }
        case OP_NOT: {
            stack_top[-1] = !stack_top[-1];
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
            if (!stack_top[-1])
                ip += offset;
            break;
        }
        case OP_JUMP_IF_TRUE: {
            u16 offset = ((*ip++) << 8) + (*ip++);
            if (stack_top[-1])
                ip += offset;
            break;
        }
        case OP_RETURN: 
            return;
        case OP_POP: {
            stack_top--;
            break;
        }
        case OP_PRINT: {
            printf("%.4f\n", stack_top[-1]);
            stack_top--;
            break;
        }
        }
    }
}