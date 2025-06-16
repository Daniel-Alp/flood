#pragma once
#include "common.h"
#define TABLE_LOAD_FACTOR (0.75)

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

// TODO implement optimization
struct ValTableEntry {
    u32 hash;
    u32 len;
    const char *chars;
    Value val;
};

struct ValTable {
    u32 cnt;
    u32 cap;
    struct ValTableEntry *entries;
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

u32 hash_string(const char *str, u32 len);

void init_val_table(struct ValTable *tab);

void release_val_table(struct ValTable *tab);

struct ValTableEntry *get_val_table_slot(
    struct ValTableEntry *vals, 
    u32 cap, 
    u32 hash, 
    u32 strlen, 
    const char *str);

void insert_val_table(struct ValTable *tab, const char *str, u32 len, Value val);

bool val_eq(Value val1, Value val2);

void print_val(Value val);