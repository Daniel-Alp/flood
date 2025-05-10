#pragma once
#include "common.h"
#include "value.h"

struct Chunk {
    u32 count;
    u32 cap;
    u8 *code;
    struct ValArray constants;
};

enum OpCode {
    // if a variable is declared but not initialized we need to make space for it on the stack
    OP_DUMMY,
    OP_TRUE,
    OP_FALSE,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    // TODO bench
    OP_LT,
    OP_LEQ,
    OP_GT,  
    OP_GEQ,
    OP_EQ_EQ,
    OP_NEQ,
    OP_NEGATE,
    OP_NOT,

    OP_GET_CONST,
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    // TODO bench
    OP_JUMP_IF_TRUE,
    OP_RETURN,

    OP_POP,
    // TEMP remove when we add functions
    OP_PRINT,
};

void init_chunk(struct Chunk *chunk);

void release_chunk(struct Chunk *chunk);

void emit_byte(struct Chunk *chunk, u8 byte);

u32 add_constant(struct Chunk *chunk, Value val);