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
    arr->vals = allocate(arr->cap * sizeof(Value));
}

void release_val_array(struct ValArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 0;
    release(arr->vals);
    arr->vals = NULL;
}

u32 push_val_array(struct ValArray *arr, Value val) 
{
    // TODO error if cnt >= 256
    // TODO add LOAD_CONST_LONG opcode
    if (arr->cnt == arr->cap) {
        arr->cap *= 2;
        arr->vals = reallocate(arr->vals, arr->cap * sizeof(Value));
    }
    arr->vals[arr->cnt] = val;
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
    case VAL_NUM:  printf("%.14g", AS_NUM(val)); break;
    case VAL_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
    case VAL_NIL:  printf("null"); break; 
    case VAL_OBJ: 
        if (AS_OBJ(val)->printed) {
            printf("...");
            return;
        } 

        AS_OBJ(val)->printed = 1;
        if (IS_FN(val)) {
            const char *name = AS_FN(val)->name;
            printf("<function %s>", name);
        } else if (IS_LIST(val)) {
            printf("[");
            struct ListObj *list = AS_LIST(val);
            Value *vals = list->vals;
            u32 cnt = list->cnt;
            if (cnt > 0) {
                for (i32 i = 0; i < cnt-1; i++) {
                    print_val(vals[i]);
                    printf(", ");
                }
                print_val(vals[cnt-1]);
            }
            printf("]");
        }
        AS_OBJ(val)->printed = 0;
    break;
    }
}