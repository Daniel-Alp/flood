#include <stdlib.h>
#include "value.h"
#include "memory.h"

void init_valarray(struct ValArray *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->values = allocate(arr->cap * sizeof(Value));
}

void release_valarray(struct ValArray *arr) {
    arr->count = 0;
    arr->cap = 0;
    release(arr->values);
    arr->values = NULL;
}

u32 push_valarray(struct ValArray *arr, Value val) {
    // TODO error if num constants exceeds 256 (later I'll add LOAD_CONSTANT_LONG)
    if (arr->cap == arr->count) {
        arr->cap *= 2;
        arr->values = reallocate(arr->values, arr->cap * sizeof(Value));
    }
    arr->values[arr->count] = val;
    arr->count++;
    return arr->count-1;
}

bool values_equal(Value val1, Value val2) {
    if (val1.tag != val2.tag)
        return false;
    switch (val1.tag) {
        case VAL_BOOL: return AS_BOOL(val1) == AS_BOOL(val2);
        case VAL_NUM: return AS_NUM(val1) == AS_NUM(val2);
        case VAL_NIL: return true;
    }
}