#pragma once
#include "vm.h"

#define GC_WHITE (0)
#define GC_GRAY  (1)
#define GC_BLACK (1 << 1)

void collect_garbage(VM &vm);

template <typename T, typename... Args>
T *alloc(VM &vm, Args... args)
    requires requires(T *t) { static_cast<Obj *>(t); }
{
    T *p = new T(forward<Args>(args)...);
    p->next = vm.obj_list;
    vm.obj_list = p;
    return p;
}
