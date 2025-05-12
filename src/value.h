#pragma once
#include "common.h"

struct Obj;

enum ValTag {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUM,
    VAL_OBJ
};

// TODO add NaN tagging
typedef struct {
    enum ValTag tag;
    union {
        bool boolean;
        double number;
        struct Obj *obj;
    } as;
} Value;

struct ValArray {
    u32 count;
    u32 cap;
    Value *values;
};

#define NIL_VAL             ((Value){VAL_NIL, {.number = 0}})
#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}})
#define NUM_VAL(value)      ((Value){VAL_NUM, {.number = value}})
#define OBJ_VAL(value)      ((Value){VAL_OBJ, {.obj = value}})

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUM(value)       ((value).as.number)
#define AS_OBJ(value)       ((value).as.obj)

#define IS_NIL(value)       ((value).tag == VAL_NIL)
#define IS_BOOL(value)      ((value).tag == VAL_BOOL)
#define IS_NUM(value)       ((value).tag == VAL_NUM)
#define IS_OBJ(value)       ((value).tag == VAL_OBJ)

void init_valarray(struct ValArray *arr);

void release_valarray(struct ValArray *arr);

u32 push_valarray(struct ValArray *arr, Value val);

bool values_equal(Value val1, Value val2);