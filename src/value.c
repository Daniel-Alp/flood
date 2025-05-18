#include <stdlib.h>
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