#pragma once
#include "parser.h"
#include "symtable.h"

void emit_error(struct Token token, const char *msg);

void emit_parse_error(struct Parser *parser, const char *msg);

void emit_resolver_error(struct Resolver *resolver, struct Token token, const char *msg);