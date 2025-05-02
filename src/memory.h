#pragma once
#include "common.h"

void *reallocate(void *ptr, u64 new_size);

void *allocate(u64 size);

void release(void *ptr);