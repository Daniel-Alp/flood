#include <stdlib.h>
#include "parser.h"
#include "memory.h"


void init_chunk(struct Chunk *chunk) {
    chunk->count = 0;
    chunk->cap = 8;
    chunk->code = reallocate(NULL, 0, 8 * sizeof(uint8_t));
}

void free_chunk(struct Chunk *chunk) {
    free(chunk->code);
    chunk->code = NULL;
    chunk->count = 0;
    chunk->cap = 0;
}

void write_chunk(struct Chunk *chunk, uint8_t opcode) {
    if (chunk->count == chunk->cap) {
        reallocate(chunk->code, chunk->cap * sizeof(uint8_t), chunk->cap * 2 * sizeof(uint8_t));
        chunk->cap *= 2;
    }
    chunk->code[chunk->count] = opcode;
    chunk->count++;
}