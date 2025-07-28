#include <string.h>
#include "memory.h"
#include "object.h"
#include "foreign.h"

// TODO implement string interning (and later symbol interning to replace string interning)
static void define_foreign_method(
    struct VM *vm,
    struct ValTable *table,     // instance method table
    char *c_str,                // chars of method name
    i32 arity,                  // arity of fn (includes `self`)
    ForeignFn code
) {
    struct ForeignFnObj *f_fn = (struct ForeignFnObj*)alloc_vm_obj(vm, sizeof(struct ForeignFnObj), NULL);
    init_foreign_fn_obj(f_fn, string_from_c_str(vm, c_str), arity, code);
    Value val = MK_OBJ((struct Obj*)f_fn);
    // we're recalculating strlen here but it should be fine
    insert_val_table(table, c_str, strlen(c_str), val);
}

// TODO when we allow ourselves to call Flood from C this is going to need to be reworked

static bool list_push(struct VM *vm)
{
    // TODO consider passing arity as an argument
    const i32 arg_count = 2;
    Value *bp = vm->sp-(arg_count+1);

    struct ListObj *list = AS_LIST(bp[2]);
    if (list->cnt == list->cap) {
        list->cap *= 2;
        list->vals = reallocate(list->vals, list->cap * sizeof(Value));
    }
    list->vals[list->cnt] = bp[1];
    list->cnt++;
    bp[0] = MK_NULL;
    return true;
}

// pop element and return it
static bool list_pop(struct VM *vm)
{
    const i32 arg_count = 1;
    Value *bp = vm->sp-(arg_count+1);

    struct ListObj *list = AS_LIST(bp[1]);
    if (list->cnt == 0) {
        runtime_err(NULL, vm, "pop from empty list");
        return false;
    }
    list->cnt--;
    bp[0] = list->vals[list->cnt]; 
    return true;
}

static bool list_length(struct VM *vm)
{
    const i32 arg_count = 1;
    Value *bp = vm->sp-(arg_count+1);

    struct ListObj *list = AS_LIST(bp[1]);
    bp[0] = MK_NUM(list->cnt);
    return true;
}

void define_list_methods(struct VM *vm)
{
    vm->list_class = (struct ClassObj*)alloc_vm_obj(vm, sizeof(struct ClassObj), NULL);
    init_class_obj(vm->list_class, string_from_c_str(vm, "List"));

    define_foreign_method(vm, &vm->list_class->methods, "push", 2, list_push);
    define_foreign_method(vm, &vm->list_class->methods, "pop", 1, list_pop);
    define_foreign_method(vm, &vm->list_class->methods, "len", 1, list_length);
}