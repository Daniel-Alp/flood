#pragma once
#include "../src/common.h"

struct Arena {
    u8 *stack_memory;
    u64 stack_pos;
};

void init_arena(struct Arena *arena);

void free_arena(struct Arena *arena);

void *arena_push(struct Arena *arena, u64 size);

void arena_pop(struct Arena *arena, u64 size);

void *arena_clear(struct Arena *arena);