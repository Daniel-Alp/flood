#pragma once
#include "chunk.h"
#include "dynarr.h"
#include "gc.h"
#include "string_symbol.h"
#include "value.h"

enum ObjTag {
    OBJ_FOREIGN_FN,
    OBJ_FN,
    OBJ_HEAP_VAL,
    OBJ_CLOSURE,
    OBJ_LIST,
    OBJ_STRING,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_METHOD,
    OBJ_FOREIGN_METHOD
};

struct Obj {
    ObjTag tag;
    u8 color;
    u8 printed;
    Obj *next;
    Obj(const ObjTag tag) : tag(tag), color(GC_WHITE), printed(0), next(nullptr) {}
    virtual ~Obj() {}
};

struct ClassObj;

// foreign fn returns true if function executed successfully, false otherwise
// preconditions:
//      - sp stored into vm->sp
//      - ip stored into frame->ip (top frame of call stack)
//      - correct number of arguments given
typedef bool (*ForeignFn)(VM &vm);

struct StringObj;

struct ForeignFnObj : public Obj {
    String name;
    ForeignFn code;
    i32 arity;
    ForeignFnObj(String &&name, ForeignFn code, i32 arity)
        : Obj(OBJ_FOREIGN_FN), name(move(name)), code(code), arity(arity)
    {
    }
};

struct FnObj : public Obj {
    String name;
    // StringObj *name;
    Chunk chunk;
    i32 arity;
    FnObj(String &&name, Chunk &&chunk, i32 arity) : Obj(OBJ_FN), name(move(name)), chunk(move(chunk)), arity(arity) {}
};

// TODO for f_method and method consider storing a copy fn instead of a pointer
struct ForeignMethodObj : public Obj {
    Obj *self;
    ForeignFnObj *fn;
    ForeignMethodObj(Obj *self, ForeignFnObj *fn) : Obj(OBJ_FOREIGN_METHOD), self(self), fn(fn) {}
};

struct HeapValObj : public Obj {
    Value val;
    HeapValObj(Value val) : Obj(OBJ_HEAP_VAL), val(val) {}
};

// FIXME need to implement move constructor and copy constructor because the default one doesn't work
struct ClosureObj : public Obj {
    FnObj *fn;
    // every element of this array is a pointer to a heap allocated Val
    u8 capture_cnt;
    HeapValObj **captures;
    ClosureObj(FnObj *fn, u8 capture_cnt)
        : Obj(OBJ_CLOSURE), fn(fn), capture_cnt(capture_cnt), captures(new HeapValObj *[capture_cnt])
    {
    }
    ~ClosureObj()
    {
        delete[] captures;
    }
};

struct ListObj : public Obj {
    Dynarr<Value> vals;
    ListObj(Dynarr<Value> &&vals) : Obj(OBJ_LIST), vals(move(vals)) {}
};

struct StringObj : public Obj {
    String str;
    StringObj(String &&str) : Obj(OBJ_STRING), str(move(str)) {}
};

struct ClassObj : public Obj {
    String name;
    ValTable methods;
    ClassObj(String &&name) : Obj(OBJ_CLASS), name(name) {}
};

struct InstanceObj : public Obj {
    ClassObj *klass;
    ValTable fields;
    InstanceObj(ClassObj *klass) : Obj(OBJ_INSTANCE), klass(klass) {}
};

struct MethodObj : public Obj {
    InstanceObj *self;
    ClosureObj *closure;
    MethodObj(InstanceObj *self, ClosureObj *closure) : Obj(OBJ_METHOD), self(self), closure(closure) {}
};

static inline bool is_obj_tag(Value val, enum ObjTag tag)
{
    return IS_OBJ(val) && AS_OBJ(val)->tag == tag;
}

#define IS_FOREIGN_FN(val)     (is_obj_tag(val, OBJ_FOREIGN_FN))
#define IS_FN(val)             (is_obj_tag(val, OBJ_FN))
#define IS_HEAP_VAL(val)       (is_obj_tag(val, OBJ_HEAP_VAL))
#define IS_CLOSURE(val)        (is_obj_tag(val, OBJ_CLOSURE))
#define IS_LIST(val)           (is_obj_tag(val, OBJ_LIST))
#define IS_STRING(val)         (is_obj_tag(val, OBJ_STRING))
#define IS_CLASS(val)          (is_obj_tag(val, OBJ_CLASS))
#define IS_INSTANCE(val)       (is_obj_tag(val, OBJ_INSTANCE))
#define IS_METHOD(val)         (is_obj_tag(val, OBJ_METHOD))
#define IS_FOREIGN_METHOD(val) (is_obj_tag(val, OBJ_FOREIGN_METHOD))

#define AS_FOREIGN_FN(val)     (static_cast<ForeignFnObj *>(AS_OBJ(val)))
#define AS_FN(val)             (static_cast<FnObj *>(AS_OBJ(val)))
#define AS_HEAP_VAL(val)       (static_cast<HeapValObj *>(AS_OBJ(val)))
#define AS_CLOSURE(val)        (static_cast<ClosureObj *>(AS_OBJ(val)))
#define AS_LIST(val)           (static_cast<ListObj *>(AS_OBJ(val)))
#define AS_STRING(val)         (static_cast<StringObj *>(AS_OBJ(val)))
#define AS_CLASS(val)          (static_cast<ClassObj *>(AS_OBJ(val)))
#define AS_INSTANCE(val)       (static_cast<InstanceObj *>(AS_OBJ(val)))
#define AS_METHOD(val)         (static_cast<MethodObj *>(AS_OBJ(val)))
#define AS_FOREIGN_METHOD(val) (static_cast<ForeignMethodObj *>(AS_OBJ(val)))
