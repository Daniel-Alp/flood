#pragma once
#include "parser.h"
#include "compiler.h"

void print_node(struct Node *node, u32 offset);

void disassemble_chunk(struct Chunk *chunk);