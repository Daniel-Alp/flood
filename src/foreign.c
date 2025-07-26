#include <string.h>
#include "memory.h"
#include "object.h"
#include "foreign.h"

// TODO implement string interning (and later symbol interning to replace string interning)
static void define_foreign_method(
    struct VM *vm,
    struct ValTable *table,     // instance method table
    char *c_str,                // chars of method name
    i32 arity,                  // arity of method
    ForeignFn code
) {
    struct ForeignFnObj *f_fn = (struct ForeignFnObj*)alloc_vm_obj(vm, sizeof(struct ForeignFnObj), NULL);
    init_foreign_fn_obj(f_fn, string_from_c_str(vm, c_str), arity, code);
    Value val = MK_OBJ((struct Obj*)f_fn);
    // we're recalculating strlen here but it should be fine
    insert_val_table(table, c_str, strlen(c_str), val);
}

// push element, return null
static bool list_push(struct VM *vm, struct Obj* self)
{
    struct ListObj *list = (struct ListObj*)self;
    if (list->cnt == list->cap) {
        list->cap *= 2;
        list->vals = reallocate(list->vals, list->cap * sizeof(Value));
    }
    list->vals[list->cnt] = vm->sp[-1];
    list->cnt++;
    vm->sp[-1] = MK_NIL;
    return true;
}

// pop element and return it
static bool list_pop(struct VM *vm, struct Obj* self)
{
    struct ListObj *list = (struct ListObj*)self;
    if (list->cnt == 0) {
        runtime_err(NULL, vm, "pop from empty list");
        return false;
    }
    list->cnt--;
    vm->sp[-1] = list->vals[list->cnt]; 
    return true;
}

static bool list_length(struct VM *vm, struct Obj *self)
{
    vm->sp[-1] = MK_NUM(((struct ListObj*)self)->cnt);
    return true;
}

void define_list_methods(struct VM *vm)
{
    vm->list_class = (struct ClassObj*)alloc_vm_obj(vm, sizeof(struct ClassObj), NULL);
    init_class_obj(vm->list_class, string_from_c_str(vm, "List"));

    define_foreign_method(vm, &vm->list_class->methods, "push", 1, list_push);
    define_foreign_method(vm, &vm->list_class->methods, "pop", 0, list_pop);
    define_foreign_method(vm, &vm->list_class->methods, "len", 0, list_length);
}