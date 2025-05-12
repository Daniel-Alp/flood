#include <stdlib.h>
#include "ty.h"
#include "memory.h"

void init_prop_array(struct PropArr *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->props = allocate(arr->cap * sizeof(struct Property));
}

void release_prop_array(struct PropArr *arr) {
    arr->count = 0;
    arr->cap = 0;
    release(arr->props);
    arr->props = NULL;
}

void push_prop_array(struct PropArr *arr, struct Property prop) {
    if (arr->count == arr->cap) {
        arr->cap *= 2;
        arr->props = reallocate(arr->props, arr->cap * sizeof(struct Property));
    }
    arr->props[arr->count] = prop;
    arr->count++;
}

// move contents of prop array onto arena and release prop array
struct Property *move_prop_array_to_arena(struct Arena *arena, struct PropArr *arr) {
    struct Property *props = push_arena(arena, arr->count * sizeof(struct Property));
    for (i32 i = 0; i < arr->count; i++)
        props[i] = arr->props[i];
    release_prop_array(arr);
    return props;
}

struct TyNode *mk_primitive_ty(struct Arena *arena, enum TyTag tag) {
    struct TyNode *ty = push_arena(arena, sizeof(struct TyNode));
    ty->tag = tag;
    return ty;
}

struct FnTyNode *mk_fn_ty(struct Arena *arena, struct Span *param_spans, struct TyNode **param_tys, struct TyNode *ret_ty, u32 arity) {
    struct FnTyNode *ty = push_arena(arena, sizeof(struct FnTyNode));
    ty->base.tag = TY_FN;
    ty->param_spans = param_spans;
    ty->param_tys = param_tys;
    ty->ret_ty = ret_ty;
    ty->arity = arity;
    return ty;
}

struct FileTyNode *mk_file_ty(struct Arena *arena, struct Property *props, u32 count) {
    struct FileTyNode *ty = push_arena(arena, sizeof(struct FileTyNode));
    ty->base.tag = TY_FILE;
    ty->prop_count = count;
    ty->props = props;
    return ty;
}

// LOOKATME!
bool cmp_ty(struct TyNode *ty1, struct TyNode *ty2) {
    if (ty1->tag != ty2->tag)
        return false;
    return true;
}

// LOOKATME!
struct TyNode *cpy_ty(struct Arena *arena, struct TyNode *ty) {
    switch (ty->tag) {
    case TY_NUM:
    case TY_BOOL:
    case TY_ANY:
    case TY_VOID:
        return mk_primitive_ty(arena, ty->tag);
    }
}