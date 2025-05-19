#include <stdio.h>
#include <stdlib.h>
#include "scan.h"
#include "object.h"
#include "value.h"
#include "memory.h"

void init_val_array(struct ValArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 8;
    arr->values = allocate(arr->cap * sizeof(Value));
}

void release_val_array(struct ValArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 0;
    release(arr->values);
    arr->values = NULL;
}

u32 push_val_array(struct ValArray *arr, Value val) 
{
    // TODO error if cnt >= 256
    // TODO add LOAD_CONST_LONG opcode
    if (arr->cnt == arr->cap) {
        arr->cap *= 2;
        arr->values = reallocate(arr->values, arr->cap * sizeof(Value));
    }
    arr->values[arr->cnt] = val;
    arr->cnt++;
    return arr->cnt-1;
}

bool val_eq(Value val1, Value val2) 
{
    if (val1.tag != val2.tag)
        return false;
    switch (val1.tag) {
        case VAL_BOOL: return AS_BOOL(val1) == AS_BOOL(val2);
        case VAL_NUM: return AS_NUM(val1) == AS_NUM(val2);
        case VAL_NIL: return true;
    }
}

void print_val(Value val) 
{
    switch (val.tag) {
    case VAL_NUM:  printf("%.4f\n", AS_NUM(val)); break;
    case VAL_BOOL: printf("%s\n", AS_BOOL(val) ? "true" : "false"); break;
    case VAL_NIL:  printf("null\n"); break; 
    case VAL_OBJ: 
        if (IS_FN(val)) {
            const char *name = AS_FN(val)->name;
            printf("<function %s>\n", name);
        }
    break;
    }
}