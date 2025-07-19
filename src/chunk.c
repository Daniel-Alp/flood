#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void init_chunk(struct Chunk *chunk) 
{
    chunk->cnt = 0;
    chunk->cap = 8;
    chunk->code = allocate(chunk->cap * sizeof(u8));
    chunk->lines_cnt = 0;
    chunk->lines_cap = 8;
    chunk->lines = allocate(chunk->lines_cap * sizeof(i32));
    init_val_array(&chunk->constants);
}

void release_chunk(struct Chunk *chunk) 
{
    chunk->cnt = 0;
    chunk->cap = 0;
    release(chunk->code);
    chunk->code = NULL;
    
    chunk->lines_cnt = 0;
    chunk->lines_cap = 0;
    release(chunk->lines);
    chunk->lines = NULL;

    release_val_array(&chunk->constants);
}

void emit_byte(struct Chunk *chunk, u8 byte, i32 line) 
{
    if (chunk->lines_cnt == 0 || chunk->lines[chunk->lines_cnt-2] != line) {
        if (chunk->lines_cnt + 2 >= chunk->lines_cap) {
            chunk->lines_cap *= 2;
            chunk->lines = reallocate(chunk->lines, chunk->lines_cap * sizeof(i32));
        }
        chunk->lines_cnt += 2;
        chunk->lines[chunk->lines_cnt-2] = line;
        chunk->lines[chunk->lines_cnt-1] = 1;
    } else {
        chunk->lines[chunk->lines_cnt-1]++;
    }

    if (chunk->cnt == chunk->cap) {
        chunk->cap *= 2;
        chunk->code = reallocate(chunk->code, chunk->cap * sizeof(u8));
    }
    chunk->code[chunk->cnt] = byte;
    chunk->cnt++;
}

i32 add_constant(struct Chunk *chunk, Value val) 
{
    return push_val_array(&chunk->constants, val);
}