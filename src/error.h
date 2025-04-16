#pragma once
#include "parser.h"

void emit_error(struct Token token, const char *source, const char *msg);

void emit_parse_error(struct Token token, const char *msg, struct Parser *parser);