#pragma once
#include "common.h"

struct Arena {
    u8 *stack_mem;
    u64 stack_pos;
};

void init_arena(struct Arena *arena);

void release_arena(struct Arena *arena);

void *push_arena(struct Arena *arena, const u64 size);
