#pragma once
#include "parse.h"
#include "chunk.h"
#include "vm.h"

void print_node(const Node *node, const i32 offset);

const char* opcode_str(const OpCode op);

void disassemble_chunk(const Chunk &chunk, const char *name);

void print_stack(const VM &vm, const Value *sp, const Value *bp);

// extern const char *opcode_str[];
