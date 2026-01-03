#include "arena.h"

Arena::~Arena() 
{
    delete[] stack_mem;
    stack_mem = nullptr;
    stack_pos = 0;
}

u8 *Arena::push(const i64 size) 
{
    if (stack_pos + size > ARENA_SIZE) {
        printf("arena out of memory\n");
        exit(1);
    }
    u8 *ptr = stack_mem + stack_pos;
    stack_pos += size;
    return ptr;
}
