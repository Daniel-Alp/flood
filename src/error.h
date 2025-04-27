#pragma once
#include "parse.h"
#include "resolve.h"

void emit_error(struct Token token, const char *msg);

void emit_parse_error(struct Parser *parser, const char *msg);

void emit_resolver_error(struct Resolver *resolver, struct Token token, const char *msg);

void emit_ty_error_unary(struct Token token, struct Ty *ty, bool *had_error);

void emit_ty_error_binary(struct Token token, struct Ty *ty_lhs, struct Ty *ty_rhs, bool *had_error);

void emit_ty_error_if(struct Span span, struct Ty *ty_thn, struct Ty *ty_els, bool *had_error);

void emit_ty_error_if_cond(struct Span span, struct Ty *ty_cond, bool *had_error);

void emit_ty_error_uninitialized(struct Token token, bool *had_error);

void emit_ty_error_cannot_infer(struct Span span);