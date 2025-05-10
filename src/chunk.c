#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void init_chunk(struct Chunk *chunk) {
    chunk->count = 0;
    chunk->cap = 8;
    chunk->code = allocate(chunk->cap * sizeof(u8));
    init_valarray(&chunk->constants);
}

void release_chunk(struct Chunk *chunk) {
    chunk->count = 0;
    chunk->cap = 0;
    release(chunk->code);
    chunk->code = NULL;
    release_valarray(&chunk->constants);
}

void emit_byte(struct Chunk *chunk, u8 byte) {
    if (chunk->cap == chunk->count) {
        chunk->cap *= 2;
        chunk->code = reallocate(chunk->code, chunk->cap * sizeof(u8));
    }
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

u32 add_constant(struct Chunk *chunk, Value val) {
    return push_valarray(&chunk->constants, val);
}