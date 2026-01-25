#pragma once
#include "scan.h"

struct ErrMsg {
    Span span;
    const char *msg;
};

void print_errarr(const Dynarr<ErrMsg> &errarr, const bool color);
