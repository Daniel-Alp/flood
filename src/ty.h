#pragma once
#include "common.h"

enum TyKind {
    TY_ANY,
    TY_NUM,
    TY_BOOL,
    TY_UNIT,
    TY_ERR
};

// Types are represented by arrays
// The type `List[List[Num]]` would be 
//
//      TY_NUM TY_LIST TY_LIST
//
// The type Map[Str, List[Num]] would be
// 
//      TY_NUM TY_LIST TY_STR TY_MAP
//
// The type Tuple(Num, Num, Str) would be 
// 
//      TY_STR TY_NUM TY_NUM 3 TY_TUP
//
struct Ty {
    u32 count; 
    u32 cap;
    u32 *arr;
};

void init_ty(struct Ty *ty);

void free_ty(struct Ty *ty);

struct Ty mk_primitive(enum TyKind primitive);

void push_ty(struct Ty *ty, u32 val);

void append_ty(struct Ty *ty_tgt, struct Ty *ty_src);

void cpy_ty(struct Ty *ty_tgt, struct Ty *ty_src);

bool cmp_ty(struct Ty *ty1, struct Ty* ty2);

u32 ty_head(struct Ty *ty);

struct SymTable;
bool resolve_tys(struct SymTable *st, struct Node *node);