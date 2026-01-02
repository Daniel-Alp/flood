#pragma once
#include "scan.h"

void init_errarr(struct ErrArr *errarr);

void release_errarr(struct ErrArr *errarr);

void push_errarr(struct ErrArr *errarr, const struct Span span, const char *msg);

void print_errarr(struct ErrArr *errarr, const bool color);
