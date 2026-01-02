#pragma once
#include "vm.h"

#define GC_WHITE (0)
#define GC_GRAY  (1)
#define GC_BLACK (1 << 1)

// currently a simple mark and sweep gc
void collect_garbage(struct VM *vm);
