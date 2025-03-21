#pragma once
#include "parser.h"
#include "compiler.h"

void print_expr(struct Node *expr, u32 offset);

void disassemble_chunk(struct Chunk *chunk);