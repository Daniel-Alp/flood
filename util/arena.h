#pragma once
#include "../src/common.h"

struct Arena {
    u8 *stack_memory;
    u64 stack_pos;
};

void arena_init(struct Arena *arena);

void arena_free(struct Arena *arena);

void *arena_push(struct Arena *arena, u64 size);

void *arena_clear(struct Arena *arena);