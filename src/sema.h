#pragma once
#include "../libflood/arena.h"
#include "ast.h"
#include "error.h"

void analyze(ModuleNode &node, Dynarr<ErrMsg> &errarr, Arena &arena);