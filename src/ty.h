#pragma once
#include "arena.h"

enum TyTag {
    TY_NUM,
    TY_BOOL,
    TY_VOID,
    TY_ANY,
};

struct TyNode {
    enum TyTag tag;
};

struct TyNode *mk_primitive_ty(struct Arena *arena, enum TyTag tag);

bool cmp_ty(struct TyNode *ty1, struct TyNode *ty2);

struct TyNode *cpy_ty(struct Arena *arena, struct TyNode *ty);