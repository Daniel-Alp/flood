#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#define ARENA_SIZE (1 << 30)

class Arena {
    u8 *stack_mem;
    u64 stack_pos;
public:
    Arena(): stack_mem(new u8[ARENA_SIZE]), stack_pos(0) {}
    ~Arena();
    u8 *push(const i64 size);
};
