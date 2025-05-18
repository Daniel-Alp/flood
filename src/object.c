#include <stdlib.h>
#include "object.h"

void init_fn_obj(struct FnObj *fn, struct Span span, u32 arity) 
{
    fn->base.tag = OBJ_FN;
    init_chunk(&fn->chunk);
    fn->span = span;
    fn->arity = arity;
}

void release_fn_obj(struct FnObj *fn) 
{
    release_chunk(&fn->chunk);
    fn->arity = 0;
}