#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void init_chunk(struct Chunk *chunk) 
{
    chunk->cnt = 0;
    chunk->cap = 8;
    chunk->code = allocate(chunk->cap * sizeof(u8));
    init_val_array(&chunk->constants);
}

void release_chunk(struct Chunk *chunk) 
{
    chunk->cnt = 0;
    chunk->cap = 0;
    release(chunk->code);
    chunk->code = NULL;
    release_val_array(&chunk->constants);
}

void emit_byte(struct Chunk *chunk, u8 byte) 
{
    if (chunk->cnt == chunk->cap) {
        chunk->cap *= 2;
        chunk->code = reallocate(chunk->code, chunk->cap * sizeof(u8));
    }
    chunk->code[chunk->cnt] = byte;
    chunk->cnt++;
}

u32 add_constant(struct Chunk *chunk, Value val) 
{
    return push_val_array(&chunk->constants, val);
}