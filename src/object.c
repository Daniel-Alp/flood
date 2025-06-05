#include <stdlib.h>
#include "object.h"
#include "memory.h"

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
}

void release_list_obj(struct ListObj *list)
{
    u32 cap = 0;
    u32 cnt = 0;
    release(list->vals);
    list->vals = NULL;
}