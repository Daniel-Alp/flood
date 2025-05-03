#include "ty.h"

struct TyNode *mk_primitive_ty(struct Arena *arena, enum TyTag tag) {
    struct TyNode *node = push_arena(arena, sizeof(struct TyNode));
    node->tag = tag;
    return node;
}

bool cmp_ty(struct TyNode *ty1, struct TyNode *ty2) {
    if (ty1->tag != ty2->tag)
        return false;
    return true;
}

struct TyNode *cpy_ty(struct Arena *arena, struct TyNode *ty) {
    switch (ty->tag) {
    case TY_NUM:
    case TY_BOOL:
    case TY_ERR:
        return mk_primitive_ty(arena, ty->tag);
    }
}