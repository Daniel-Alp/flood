#pragma once
#include "scan.h"

void init_errlist(struct ErrList *errlist);

void release_errlist(struct ErrList *errlist);

void push_errlist(struct ErrList *errlist, struct Span span, const char *msg);

void print_errlist(struct ErrList *errlist, bool color);