#include <stdlib.h>
#include "object.h"

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
    fn->arity = 0;
}