#pragma once
#include "../libflood/dynarr.h"
#include "value.h"

// TODO add LONG equivalents for GET/SET bytecodes
enum OpCode {
    OP_NULL,
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

    // TODO make OP_LIST_LONG which can more elems
    OP_LIST, // args: n 0..=255
    // args: n 0..=255, then n indices 0..=255 offset from bp
    // then  m 0..=255, then m indices 0..=255 idx into capture arr
    OP_HEAPVAL,      // args: 0..=255 offset from bp
    OP_CLOSURE,
    OP_CLASS,
    OP_METHOD,

    OP_GET_CONST,    // args: 0..=255 idx into constant arr
    OP_GET_LOCAL,    // args: 0..=255 offset from bp
    OP_SET_LOCAL,    // args: 0..=255 offset from bp
    OP_GET_HEAPVAL,  // args: 0..=255 offset from bp
    OP_SET_HEAPVAL,  // args: 0..=255 offset from bp
    OP_GET_CAPTURED, // args: 0..=255 idx into capture arr
    OP_SET_CAPTURED, // args: 0..=255 idx into capture arr
    // TEMP remove globals when we added user-defined classes
    OP_GET_GLOBAL, // args: 0..=255 idx into global arr
    OP_SET_GLOBAL, // args: 0..=255 idx into global arr
    OP_GET_SUBSCR,
    OP_SET_SUBSCR,

    OP_GET_FIELD, // args: 0..=255 idx into constant arr
    OP_SET_FIELD, // args: 0..=255 idx into constant arr

    OP_GET_METHOD, // args: 0..=255 idx into constant arr

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_CALL,
    OP_RETURN,

    OP_POP,
    OP_POP_N, // args: index 0..=255
    // TEMP remove when we add functions
    OP_PRINT,
};

class Chunk {
    // NOTE:
    // representing line info
    //      var x;
    //      x = 1 + 2;
    // the bytecode would be
    //      OP_NULL
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
    //      8       opcode cnt
    Dynarr<i32> lines_;
    Dynarr<u8> code_;
    Dynarr<Value> constants_;

public:
    Dynarr<i32> const &lines() const
    {
        return lines_;
    }
    Dynarr<u8> const &code() const
    {
        return code_;
    }
    Dynarr<u8> &code()
    {
        return code_;
    }
    Dynarr<Value> const &constants() const
    {
        return constants_;
    }
    void emit_byte(const u8 byte, const i32 line);
    i32 add_constant(const Value val);
};
