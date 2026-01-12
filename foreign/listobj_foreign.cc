#include "../src/foreign.h"

static void list_push(Value val, ListObj *self)
{
    self->vals.push(val);
}

static Value list_pop(ListObj *self)
{
    if (self->vals.len() == 0)
        throw "pop from empty list";
    Value val = self->vals[self->vals.len()-1];
    self->vals.pop();
    return val;
}

static double list_len(ListObj *self)
{
    return self->vals.len();
}

void define_list_methods(VM &vm)
{
    define_foreign_method<list_push>(vm, *vm.list_class, "push");
    define_foreign_method<list_len>(vm, *vm.list_class, "len");
    define_foreign_method<list_pop>(vm, *vm.list_class, "pop");
}
