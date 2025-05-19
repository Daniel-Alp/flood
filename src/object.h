#pragma once
#include "chunk.h"
#include "scan.h"

enum ObjTag {
    OBJ_FN,
};

struct Obj {
    enum ObjTag tag;
    struct Obj *next;
};

struct FnObj {
    struct Obj base;
    char *name;
    struct Chunk chunk; 
    u32 arity;
};

static inline bool is_obj_tag(Value val, enum ObjTag tag) 
{
    return IS_OBJ(val) && AS_OBJ(val)->tag == tag;
}

#define IS_FN(val) (is_obj_tag(val, OBJ_FN))

#define AS_FN(val) ((struct FnObj*)AS_OBJ(val))

void init_fn_obj(struct FnObj *fn, const char *name, u32 arity);

void release_fn_obj(struct FnObj *fn);