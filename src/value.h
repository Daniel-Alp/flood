#pragma once
#include "common.h"
#define TABLE_LOAD_FACTOR (0.75)

struct Obj;
struct StringObj;

enum ValTag { VAL_NULL, VAL_BOOL, VAL_NUM, VAL_OBJ };

// TODO NaN tagging
typedef struct {
    enum ValTag tag;
    union {
        bool boolean;
        double number;
        struct Obj *obj;
    } as;
} Value;

#define MK_NULL      ((Value){VAL_NULL, {.number = 0}})
#define MK_BOOL(val) ((Value){VAL_BOOL, {.boolean = val}})
#define MK_NUM(val)  ((Value){VAL_NUM, {.number = val}})
#define MK_OBJ(val)  ((Value){VAL_OBJ, {.obj = val}})

#define AS_BOOL(val) ((val).as.boolean)
#define AS_NUM(val)  ((val).as.number)
#define AS_OBJ(val)  ((val).as.obj)

#define IS_NULL(val) ((val).tag == VAL_NULL)
#define IS_BOOL(val) ((val).tag == VAL_BOOL)
#define IS_NUM(val)  ((val).tag == VAL_NUM)
#define IS_OBJ(val)  ((val).tag == VAL_OBJ)

bool val_eq(const Value val1, const Value val2);

void print_val(Value val);

struct Assoc {
    StringObj *key = nullptr;
    Value val = MK_NULL;
};

class ValTable {
    i32 cnt;
    i32 cap;
    Assoc *vals;

    static Assoc &find_slot(StringObj &key, Assoc *vals, const i32 cap);

public:
    ValTable() : cnt(0), cap(8), vals(static_cast<Assoc *>(new Assoc[cap])){};
    ~ValTable()
    {
        delete[] vals;
    }

    void insert(StringObj &key, Value val);

    Value *find(StringObj &key);
};
