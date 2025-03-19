#include <stdlib.h>
#include "arena.h"
#include "memory.h"

void init_arena(struct Arena *arena) {
    arena->stack_memory = allocate(4294967296);
    arena->stack_pos = 0;
}

void free_arena(struct Arena *arena) {
    free(arena->stack_memory);
    arena->stack_memory = NULL;
    arena->stack_pos = 0;
}

void *arena_push(struct Arena *arena, u64 size) {
    void *ptr = arena->stack_memory + arena->stack_pos;
    arena->stack_pos += size;
    return ptr;
}

void *arena_clear(struct Arena *arena) {
    arena->stack_pos = 0;
}