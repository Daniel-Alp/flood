#pragma once
#include "parse.h"
#include "chunk.h"
#include "vm.h"

void print_node(struct Node *node, u32 offset);

void disassemble_chunk(struct Chunk *chunk, struct Span span);

void print_stack(struct VM *vm, Value *sp, Value *bp);

extern const char *opcode_str[];