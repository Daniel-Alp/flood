#pragma once
#include "ast.h"
#include "vm.h"

void print_module(ModuleNode &node, const bool verbose);

const char *opcode_str(const OpCode op);

void disassemble_chunk(const Chunk &chunk, const char *name);

void print_stack(const VM &vm, const Value *sp, const Value *bp);