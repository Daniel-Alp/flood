#include <stdio.h>
#include <stdlib.h>
#include "arena.h"
#include "memory.h"

#define ARENA_SIZE (1 << 30)

void init_arena(struct Arena *arena) 
{
    arena->stack_mem = allocate(ARENA_SIZE);
    arena->stack_pos = 0;
}

void release_arena(struct Arena *arena) 
{
    release(arena->stack_mem);
    arena->stack_mem = NULL;
    arena->stack_pos = 0;
}

void *push_arena(struct Arena *arena, u64 size) 
{
    if (arena->stack_pos + size > ARENA_SIZE) {
        printf("arena out of memory\n");
        exit(1);
    }
    void *ptr = arena->stack_mem + arena->stack_pos;
    arena->stack_pos += size;
    return ptr;
}