#pragma once
#include "dynarr.h"
#include "scan.h"

struct ErrMsg {
    struct Span span;
    const char *msg;
};

void print_errarr(const Dynarr<ErrMsg> &errarr, const bool color);
