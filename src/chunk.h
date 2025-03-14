#pragma once
#include "common.h"

typedef double Value;

struct ValueArray {
    Value *vals;
    u32 count;
    u32 cap;
};

struct Chunk {
    u8 *code;
    u32 count;
    u32 cap;
    struct ValueArray constants;
};

void init_value_array(struct ValueArray *array);

void write_value_array(struct ValueArray *array, Value val);

void free_value_array(struct ValueArray *array);

void init_chunk(struct Chunk *chunk);

void write_chunk(struct Chunk *chunk, u8 byte);

void free_chunk(struct Chunk *chunk);