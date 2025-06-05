#pragma once
#include "chunk.h"
#include "scan.h"

enum ObjTag {
    OBJ_FN,
    OBJ_LIST,
};

struct Obj {
    enum ObjTag tag;
    struct Obj *next;
};

struct FnObj {
    struct Obj base;
    const char *name;
    struct Chunk chunk; 
    u32 arity;
};

struct ListObj {
    struct Obj base;
    u32 cap;
    u32 cnt;
    Value *vals;
};

static inline bool is_obj_tag(Value val, enum ObjTag tag) 
{
    return IS_OBJ(val) && AS_OBJ(val)->tag == tag;
}

#define IS_FN(val)   (is_obj_tag(val, OBJ_FN))
#define IS_LIST(val) (is_obj_tag(val, OBJ_LIST))

#define AS_FN(val)   ((struct FnObj*)AS_OBJ(val))
#define AS_LIST(val) ((struct ListObj*)AS_OBJ(val))

void init_fn_obj(struct FnObj *fn, const char *name, u32 arity);

void release_fn_obj(struct FnObj *fn);

void init_list_obj(struct ListObj *list, Value *vals, u32 cnt);

void release_list_obj(struct ListObj *list);