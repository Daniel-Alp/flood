#include <stdlib.h>
#include "chunk.h"
#include "../util/memory.h"

void init_value_array(struct ValueArray *array) {
    array->cap = 8;
    array->count = 0;
    array->arr = allocate(8 * sizeof(Value));
}

void write_value_array(struct ValueArray *array, Value val) {
    if (array->count == array->cap) {
        array->arr = reallocate(array->arr, 2 * array->cap * sizeof(Value));
        array->cap *= 2;
    }
    array->arr[array->count] = val;
    array->count++;
}

void free_value_array(struct ValueArray *array) {
    free(array->arr);
    array->arr = NULL;
    array->count = 0;
    array->cap = 0;
}

void init_chunk(struct Chunk *chunk) {
    chunk->cap = 8;
    chunk->count = 0;
    chunk->code = allocate(8 * sizeof(u8));
    init_value_array(&chunk->constants);
}

void write_chunk(struct Chunk *chunk, u8 byte) {
    if (chunk->count == chunk->cap) {
        chunk->code = reallocate(chunk->code, 2 * chunk->cap * sizeof(u8));
        chunk->cap *= 2;
    }
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void free_chunk(struct Chunk *chunk) {
    free(chunk->code);
    chunk->code = NULL;
    chunk->count = 0;
    chunk->cap = 0;
}