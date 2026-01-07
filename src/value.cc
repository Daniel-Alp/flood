#include "value.h"
#include "dynarr.h"
#include "object.h"
#include <stdio.h>

bool val_eq(const Value val1, const Value val2)
{
    if (val1.tag != val2.tag)
        return false;
    // clang-format off
    switch (val1.tag) {
    case VAL_BOOL: return AS_BOOL(val1) == AS_BOOL(val2);
    case VAL_NUM:  return AS_NUM(val1) == AS_NUM(val2);
    case VAL_NULL: return true;
    case VAL_OBJ:  return AS_OBJ(val1) == AS_OBJ(val2); // TODO proper string comparison (?)
    }
    // clang-format on
    // TODO should never reach this case
}

void print_val(const Value val)
{
    switch (val.tag) {
    case VAL_NUM: printf("%.14g", AS_NUM(val)); break;
    case VAL_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
    case VAL_NULL: printf("null"); break;
    case VAL_OBJ:
        if (AS_OBJ(val)->printed) {
            printf("...");
            return;
        }
        AS_OBJ(val)->printed = 1;
        enum ObjTag tag = AS_OBJ(val)->tag;
        switch (tag) {
        case OBJ_FOREIGN_FN: {
            const char *name = AS_FOREIGN_FN(val)->name->str.chars();
            printf("<foreign function %s>", name);
            break;
        }
        case OBJ_FN: {
            const char *name = AS_FN(val)->name->str.chars();
            printf("<function %s>", name);
            break;
        }
        case OBJ_CLOSURE: {
            const char *name = AS_CLOSURE(val)->fn->name->str.chars();
            printf("<closure %s>", name);
            break;
        }
        case OBJ_LIST: {
            printf("[");
            struct ListObj *list = AS_LIST(val);
            const Dynarr<Value> &vals = list->vals;
            if (vals.len() > 0) {
                for (i32 i = 0; i < vals.len() - 1; i++) {
                    print_val(vals[i]);
                    printf(", ");
                }
                print_val(vals[vals.len() - 1]);
            }
            printf("]");
            break;
        }
        case OBJ_HEAP_VAL: {
            print_val(AS_HEAP_VAL(val)->val);
            break;
        }
        case OBJ_STRING: {
            printf("%s", AS_STRING(val)->str.chars());
            break;
        }
        }
        AS_OBJ(val)->printed = 0;
        break;
    }
}
