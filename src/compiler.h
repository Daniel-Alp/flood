#pragma once
#include "common.h"
#include "chunk.h"
#include "parser.h"

enum OpCode {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, 
    OP_NEGATE,
    OP_CONST,
    OP_RETURN
};

void init_chunk(struct Chunk *chunk);

void compile(struct Chunk *chunk, struct Node *expr);