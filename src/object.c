#include <string.h>
#include <stdlib.h>
#include "object.h"
#include "memory.h"

void init_foreign_method_obj(
    struct ForeignMethodObj *f_method, 
    struct Obj* self,
    struct StringObj *name,
    i32 arity,
    ForeignMethod code
) {
    f_method->base.tag = OBJ_FOREIGN_METHOD;
    f_method->self = self;
    f_method->name = name;
    f_method->arity = arity;
    f_method->code = code;
}

void release_foreign_method_obj(struct ForeignMethodObj *f_method)
{
    // method does not own foreign object

    // GC handles lifetime of name
    f_method->name = NULL;
    f_method->arity = 0;
}

void init_fn_obj(struct FnObj *fn, struct StringObj *name, i32 arity) 
{
    fn->base.tag = OBJ_FN;
    init_chunk(&fn->chunk);
    fn->name = name;
    fn->arity = arity;
}

void release_fn_obj(struct FnObj *fn) 
{
    release_chunk(&fn->chunk);
    // GC handles lifetime of name
    fn->name = NULL;
    fn->arity = 0;
}

void init_heap_val_obj(struct HeapValObj *heap_val, Value val)
{
    heap_val->base.tag = OBJ_HEAP_VAL;
    heap_val->val = val;
}

// NOTE: 
// init_closure_obj only creates fn and allocates memory for the heap vals array
// in the run_vm function the heap vals ptrs are set
void init_closure_obj(struct ClosureObj *closure, struct FnObj *fn, u8 n) 
{
    closure->base.tag = OBJ_CLOSURE;
    closure->fn = fn;
    closure->capture_cnt = n;
    closure->captures = allocate(n * sizeof(struct HeapValObj*));
}

void release_closure_obj(struct ClosureObj *closure)
{
    // the closure does not own the function
    // the closure also does not own the heap vals it has references to
    release(closure->captures);
    closure->capture_cnt = 0;
    closure->captures = NULL;
}

void init_class_obj(struct ClassObj *class, struct StringObj *name)
{
    class->base.tag = OBJ_CLASS;
    class->name = name;
    init_val_table(&class->methods);
}

void release_class_obj(struct ClassObj *class)
{
    // GC handles lifetime of name
    release_val_table(&class->methods);
}

// NOTE: vals is the methods defined on the class
void init_instance_obj(struct InstanceObj *instance, struct ClassObj *class)
{
    instance->base.tag = OBJ_INSTANCE;
    instance->class = class;
    init_val_table(&instance->props);
}

void release_instance_obj(struct InstanceObj *instance)
{
    // instance does not own class
    instance->class = NULL;
    release_val_table(&instance->props);
}

// TODO consider copying contents of closure to avoid going through pointer
void init_method_obj(struct MethodObj *method, struct ClosureObj *closure, struct InstanceObj *self)
{
    method->base.tag = OBJ_METHOD;
    method->closure = closure;
    method->self = self;
}

// TODO consider copying contents of closure to avoid going through pointer
void release_method_obj(struct MethodObj *method)
{
    method->closure = NULL;
    method->self = NULL;
}

// NOTE: vals points to the start of the list's elements in the stack 
void init_list_obj(struct ListObj *list, Value *vals, i32 cnt)
{
    list->base.tag = OBJ_LIST;
    i32 cap = 8;
    while (cnt > cap)
        cap *= 2;
    list->cnt = cnt;
    list->cap = cap;
    list->vals = allocate(sizeof(Value) * cap);
    for (i32 i = 0; i < cnt; i++)
        list->vals[i] = vals[i];
    init_val_table(&list->methods);
}

void release_list_obj(struct ListObj *list)
{
    i32 cap = 0;
    i32 cnt = 0;
    release(list->vals);
    list->vals = NULL;
    release_val_table(&list->methods);
}

void init_string_obj(struct StringObj *str, u32 hash, i32 len, char *chars)
{
    str->base.tag = OBJ_STRING;
    str->hash = hash;
    str->len = len;
    str->chars = chars;
}

void release_string_obj(struct StringObj *str)
{
    str->hash = 0;
    str->len = 0;
    release(str->chars);
    str->chars = NULL;
}