#pragma once
#include "vm.h"
#include "chunk.h"
#include "scan.h"

enum ObjTag {
    OBJ_CLASS, // DELETEME
    OBJ_FOREIGN_METHOD,
    OBJ_FN,
    OBJ_LIST,
    OBJ_STRING
};

struct Obj {
    enum ObjTag tag;
    u8 color;         
    u8 printed;
    struct Obj *next;
};

// foreign method returns true if function executed successfully, false otherwise
// preconditions:
//      - sp stored into vm->sp
//      - ip stored into frame->ip (top frame of call stack)
//      - correct number of arguments given
typedef bool(*ForeignMethod)(struct VM *vm, struct Obj* self);

struct ForeignMethodObj {
    struct Obj base;
    struct Obj *self;
    const char *name;
    ForeignMethod code;
    u32 arity;
};

// TEMP remove once we implement non-foreign methods
struct FnObj {
    struct Obj base;
    const char *name;
    struct Chunk chunk; 
    u32 arity;
};

struct ListObj {
    struct Obj base;
    struct ValTable methods;
    u32 cap;
    u32 cnt;
    Value *vals;
};

struct StringObj {
    struct Obj base;
    u32 hash;
    u32 len;
    char *chars; // has null byte at the end, but len does not count it
};

static inline bool is_obj_tag(Value val, enum ObjTag tag) 
{
    return IS_OBJ(val) && AS_OBJ(val)->tag == tag;
}

#define IS_FOREIGN_METHOD(val) (is_obj_tag(val, OBJ_FOREIGN_METHOD)) 
#define IS_FN(val)             (is_obj_tag(val, OBJ_FN))
#define IS_LIST(val)           (is_obj_tag(val, OBJ_LIST))
#define IS_STRING(val)         (is_obj_tag(val, OBJ_STRING))

#define AS_FOREIGN_METHOD(val) ((struct ForeignMethodObj*)AS_OBJ(val))
#define AS_FN(val)             ((struct FnObj*)AS_OBJ(val))
#define AS_LIST(val)           ((struct ListObj*)AS_OBJ(val))
#define AS_STRING(val)         ((struct StringObj*)AS_OBJ(val))

void init_foreign_method_obj(
    struct ForeignMethodObj *f_method, 
    struct Obj* self,
    const char *name,
    u32 arity,
    ForeignMethod code
);

void release_foreign_method_obj(struct ForeignMethodObj *f_method);

void init_fn_obj(struct FnObj *fn, const char *name, u32 arity);

void release_fn_obj(struct FnObj *fn);

void init_list_obj(struct ListObj *list, Value *vals, u32 cnt);

void release_list_obj(struct ListObj *list);

void init_string_obj(struct StringObj *str, u32 hash, u32 len, char *chars);

void release_string_obj(struct StringObj *str);