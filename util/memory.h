#pragma once
#include <stddef.h>

void *reallocate(void *pointer, size_t new_size);

void *allocate(size_t size);