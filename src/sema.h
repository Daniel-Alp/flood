#pragma once
#include "ast.h"
#include "dynarr.h"
#include "error.h"

void analyze(ModuleNode &node, Dynarr<ErrMsg> &errarr);