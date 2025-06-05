#pragma once
#include "common.h"

struct Obj;

enum ValTag {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUM,
    VAL_OBJ
};

// TODO NaN tagging
typedef struct {
    enum ValTag tag;
    union {
        bool boolean;
        double number;
        struct Obj *obj;
    } as;
} Value;

struct ValArray {
    u32 cnt;
    u32 cap;
    Value *vals;
};

#define MK_NIL            ((Value){VAL_NIL, {.number = 0}})
#define MK_BOOL(val)      ((Value){VAL_BOOL, {.boolean = val}})
#define MK_NUM(val)       ((Value){VAL_NUM, {.number = val}})
#define MK_OBJ(val)       ((Value){VAL_OBJ, {.obj = val}})

#define AS_BOOL(val)      ((val).as.boolean)
#define AS_NUM(val)       ((val).as.number)
#define AS_OBJ(val)       ((val).as.obj)

#define IS_NIL(val)       ((val).tag == VAL_NIL)
#define IS_BOOL(val)      ((val).tag == VAL_BOOL)
#define IS_NUM(val)       ((val).tag == VAL_NUM)
#define IS_OBJ(val)       ((val).tag == VAL_OBJ)

void init_val_array(struct ValArray *arr);

void release_val_array(struct ValArray *arr);

u32 push_val_array(struct ValArray *arr, Value val);

bool val_eq(Value val1, Value val2);

void print_val(Value val);