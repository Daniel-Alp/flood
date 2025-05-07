#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

static void init_valarray(struct ValArray *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->values = allocate(arr->cap * sizeof(Value));
}

static void release_valarray(struct ValArray *arr) {
    arr->count = 0;
    arr->cap = 0;
    release(arr->values);
    arr->values = NULL;
}

static u32 push_valarray(struct ValArray *arr, Value val) {
    // TODO error if num constants exceeds 256 (later I'll add LOAD_CONSTANT_LONG)
    if (arr->cap == arr->count) {
        arr->cap *= 2;
        arr->values = reallocate(arr->values, arr->cap * sizeof(Value));
    }
    arr->values[arr->count] = val;
    arr->count++;
    return arr->count-1;
}

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