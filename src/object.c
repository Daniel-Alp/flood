#include <stdlib.h>
#include "object.h"
#include "memory.h"

void init_foreign_method_obj(
    struct ForeignMethodObj *f_method, 
    struct Obj* self,
    const char *name,
    u32 arity,
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
    f_method->self = NULL; // method does not own foreign object
    release(f_method->name);
    f_method->name = NULL;
    f_method->arity = 0;
}

void init_fn_obj(struct FnObj *fn, const char *name, u32 arity) 
{
    fn->base.tag = OBJ_FN;
    init_chunk(&fn->chunk);
    fn->name = name;
    fn->arity = arity;
}

void release_fn_obj(struct FnObj *fn) 
{
    release_chunk(&fn->chunk);
    release(fn->name);
    fn->name = NULL;
    fn->arity = 0;
}

void init_list_obj(struct ListObj *list, Value *vals, u32 cnt)
{
    list->base.tag = OBJ_LIST;
    u32 cap = 8;
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
    u32 cap = 0;
    u32 cnt = 0;
    release(list->vals);
    list->vals = NULL;
    release_val_table(&list->methods);
}

void init_string_obj(struct StringObj *str, u32 hash, u32 len, char *chars)
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