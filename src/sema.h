#pragma once
#include "ast.h"
#include "error.h"

void analyze(ModuleNode &node, Dynarr<ErrMsg> &errarr);