#pragma once
#include "parser.h"

void error(struct Token token, const char *message, const char *source);

void parse_error(struct Parser *parser, const char *message);