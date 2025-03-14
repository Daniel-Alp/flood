#pragma once
#include "parser.h"
#include "compiler.h"

void print_expr(struct Expr *expr, u32 offset);

void disassemble_chunk(struct Chunk *chunk);