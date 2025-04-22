#pragma once
#include "parser.h"

void emit_error(struct Token token, const char *source, const char *msg);

void emit_parse_error(struct Parser *parser, const char *msg);