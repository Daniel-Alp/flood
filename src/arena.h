#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "dynarr.h"

#define ARENA_SIZE (1 << 30)

class Arena {
    u8 *stack_mem;
    u64 stack_pos;
public:
    Arena(): stack_mem(new u8[ARENA_SIZE]), stack_pos(0) {}
    ~Arena();
    u8 *push(const i64 size);
};

template<typename T, typename... Args>
T *alloc(Arena &arena, Args&&... args) 
{
    return new (arena.push(sizeof(T))) T(args...);
}

template<typename T>
T *move_dynarr(Arena &arena, Dynarr<T> &&dynarr)
{
    T *vals = reinterpret_cast<T*>(arena.push(dynarr.size() * sizeof(T)));
    for (i32 i = 0; i < dynarr.size(); i++) 
        new (vals + i) T(static_cast<T&&>(dynarr[i]));
    return vals;
}
