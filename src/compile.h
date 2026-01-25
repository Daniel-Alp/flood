#pragma once
#include "ast.h"
#include "error.h"
#include "object.h"

ClosureObj *compile(VM &vm, ModuleNode &node, Dynarr<ErrMsg> &errarr);
