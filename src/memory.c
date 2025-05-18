#include <stdlib.h>
#include <stddef.h>
#include "memory.h"

void *reallocate(void *ptr, u64 new_size) 
{
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    void *result = realloc(ptr, new_size);
    if (result == NULL)
        exit(1);
    return result;
}

void *allocate(u64 size)
{
    return reallocate(NULL, size);
}

void release(void *ptr) 
{
    reallocate(ptr, 0);
}