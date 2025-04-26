#pragma once
#include "parse.h"
#include "resolve.h"

void print_node(struct Node *node, u32 offset);

void print_ty(struct Ty *ty);

void print_symtable(struct SymTable *st);