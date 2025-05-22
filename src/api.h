#pragma once
#include "vm.h"

enum InterpResult do_file(struct VM *vm, const char *path);

// path is path from source root to file
i32 get_global(struct VM *vm, const char *path, const char *name);