#pragma once
#include "common.h"
#include "value.h"

struct Chunk {
    u32 cnt;
    u32 cap;
    u8 *code;
    // representing line info
    //      var x;
    //      x = 1 + 2;
    // the bytecode would be
    //      OP_NIL
    //      OP_CONST
    //      <const table idx for 1>
    //      OP_CONST
    //      <const table idx for 2>
    //      OP_ADD
    //      OP_SET_LOCAL
    //      <val stack idx for x>
    //      OP_POP
    // the line info would be
    //      1       line no
    //      1       opcode cnt
    //      2       line no
    //      8       opcode_cnt
    // this is wasteful, but good enough for now
    u32 lines_cnt; 
    u32 lines_cap;                 
    u32 *lines;
    struct ValArray constants;
};

enum OpCode {
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_FLOORDIV,
    OP_MOD,
    OP_LT,
    OP_LEQ,
    OP_GT,  
    OP_GEQ,
    OP_EQEQ,
    OP_NEQ,
    OP_NEGATE,
    OP_NOT,

    OP_LIST,          // args: index 0..=255

    OP_GET_CONST,     // args: index 0..=255
    OP_GET_LOCAL,     // args: index 0..=255 
    OP_SET_LOCAL,     // args: index 0..=255
    OP_GET_GLOBAL,    // args: index 0..=255
    OP_SET_GLOBAL,    // args: index 0..=255
    OP_GET_SUBSCR,    
    OP_SET_SUBSCR,     

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_CALL,
    OP_RETURN,

    OP_POP,
    OP_POP_N,         // args: index 0..=255
    // TEMP remove when we add functions
    OP_PRINT,
};

void init_chunk(struct Chunk *chunk);

void release_chunk(struct Chunk *chunk);

void emit_byte(struct Chunk *chunk, u8 byte, u32 line);

u32 add_constant(struct Chunk *chunk, Value val);