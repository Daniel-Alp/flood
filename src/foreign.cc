#include "foreign.h"
#include "gc.h"
#include "object.h"
#include "value.h"

static void define_foreign_method(VM &vm, ClassObj *klass, String &&str, const i32 arity, ForeignFn code)
{
    ForeignFnObj *f_fn = alloc<ForeignFnObj>(vm, str, code, arity);
    klass->methods.insert(*alloc<StringObj>(vm, str), MK_OBJ(f_fn));
}

// TODO when we allow ourselves to call Flood from C this is going to need to be reworked

static bool list_push(VM &vm)
{
    // TODO consider passing arity as an argument
    const i32 arg_count = 1;
    Value *bp = vm.sp - (arg_count + 1);

    ListObj *list = AS_LIST(bp[1]);
    list->vals.push(bp[0]);
    bp[-1] = MK_NULL;
    return true;
}

// pop element and return it
static bool list_pop(VM &vm)
{
    const i32 arg_count = 0;
    Value *bp = vm.sp - (arg_count + 1);

    ListObj *list = AS_LIST(bp[0]);
    if (list->vals.len() == 0) {
        runtime_err(NULL, vm, "pop from empty list");
        return false;
    }
    bp[-1] = list->vals[list->vals.len() - 1];
    list->vals.pop();
    return true;
}

static bool list_len(VM &vm)
{
    const i32 arg_count = 0;
    Value *bp = vm.sp - (arg_count + 1);

    ListObj *list = AS_LIST(bp[0]);
    bp[-1] = MK_NUM(double(list->vals.len()));
    return true;
}

void define_list_methods(VM &vm)
{
    define_foreign_method(vm, vm.list_class, "push", 2, list_push);
    define_foreign_method(vm, vm.list_class, "pop", 1, list_pop);
    define_foreign_method(vm, vm.list_class, "len", 1, list_len);
}
