#pragma once
#include "scan.h"

struct ErrMsg {
    struct Span span;
    const char *msg;
};

struct ErrList {
    i32 cnt;
    i32 cap;
    struct ErrMsg *errs;
};

void init_errlist(struct ErrList *errlist);

void release_errlist(struct ErrList *errlist);

void push_errlist(struct ErrList *errlist, struct Span span, const char *msg);

void print_errlist(struct ErrList *errlist, bool color);