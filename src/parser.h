#pragma once
#include "../util/arena.h"
#include "scanner.h"

struct Parser {
    struct Scanner scanner;
    struct Token current;
    struct Token next;
    bool had_error;
};

bool matching_braces(struct Scanner *scanner);

struct BlockExpr *parse(struct Arena *arena, const char *source);