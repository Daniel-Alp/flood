#pragma once
#include <stddef.h>
#include <stdlib.h>

void *reallocate(void *ptr, size_t new_size);

void *allocate(size_t size);

void release(void *ptr);