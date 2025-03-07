#pragma once
#include <stdint.h>

typedef double Value;

enum OpCode {
   OP_PLUS, OP_SUB, OP_MULT, OP_DIV,
   OP_CONSTANT,
   OP_RETURN
};

struct Chunk {
   int count;
   int cap;
   uint8_t *code;     
};

void init_chunk(struct Chunk *chunk);

void free_chunk(struct Chunk *chunk);

void write_chunk(struct Chunk *chunk, uint8_t opcode);