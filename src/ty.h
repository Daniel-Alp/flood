#pragma once
#include "arena.h"
#include "scan.h"

enum TyTag {
    TY_NUM,
    TY_BOOL,
    TY_VOID,
    TY_FN,
    TY_FILE,
    TY_NEVER,
    TY_ANY,
};

struct TyNode {
    enum TyTag tag;
};

struct Property {
    struct Span span;
    struct TyNode *ty;
};

struct PropArr {
    u32 count;
    u32 cap;
    struct Property *props;
};

struct FnTyNode {
    struct TyNode base;
    // span for each param (TODO use for error messages)
    struct Span *param_spans;
    struct TyNode **param_tys;
    struct TyNode *ret_ty;
    u32 arity;
};

// the type of a file is the functions and variables it exposes to other files
struct FileTyNode {
    struct TyNode base;
    struct Property *props;
    u32 prop_count;
};

void init_prop_array(struct PropArr *arr);

void release_prop_array(struct PropArr *arr);

void push_prop_array(struct PropArr *arr, struct Property prop);

struct Property *move_prop_array_to_arena(struct Arena *arena, struct PropArr *arr);

struct TyNode *mk_primitive_ty(struct Arena *arena, enum TyTag tag);

struct FnTyNode *mk_fn_ty(struct Arena *arena, struct Span *param_spans, struct TyNode **param_tys, struct TyNode *ret_ty, u32 arity);

struct FileTyNode *mk_file_ty(struct Arena *arena, struct Property *props, u32 count);

bool cmp_ty(struct TyNode *ty1, struct TyNode *ty2);

struct TyNode *cpy_ty(struct Arena *arena, struct TyNode *ty);