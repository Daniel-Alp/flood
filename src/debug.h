#pragma once
#include "parse.h"
#include "chunk.h"

void print_node(struct Node *node, u32 offset);

void disassemble_chunk(struct Chunk *chunk, struct Span span);