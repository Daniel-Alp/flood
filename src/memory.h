#pragma once
#include "common.h"

void *reallocate(void *ptr, const u64 new_size);

void *allocate(const u64 size);

void release(void *ptr);
