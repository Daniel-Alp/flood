#include <string.h>
#include "memory.h"
#include "object.h"
#include "foreign.h"

// TODO implement string interning
// When I do implement string interning, char *name will be the pointer to the string in the table
static void bind_foreign_method(
    struct VM *vm,
    struct Obj *self,           // instance method is bound to
    struct ValTable *table,     // instance method table
    char *name,                 // chars of method name
    u32 len,                    // length of method name
    u32 arity,                  // arity of method
    ForeignMethod code
) {
    // TODO every instance has it's own copy of the name characters, which is wasteful
    char *name_cpy = allocate((len+1)*sizeof(char));
    name_cpy[len] = '\0';
    memcpy(name_cpy, name, len);
    struct ForeignMethodObj *f_method = (struct ForeignMethodObj*)alloc_vm_obj(vm, sizeof(struct ForeignMethodObj));
    init_foreign_method_obj(f_method, self, name_cpy, arity, code);
    Value val = MK_OBJ((struct Obj*)f_method);
    insert_val_table(table, name, len, val);
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
        runtime_err(vm, "pop from empty list");
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

void bind_list_methods(struct VM *vm, struct ListObj *list)
{
    struct Obj *obj = (struct Obj*)list;
    bind_foreign_method(vm, obj, &list->methods, "push", 4, 1, list_push);
    bind_foreign_method(vm, obj, &list->methods, "pop", 3, 0, list_pop);
    bind_foreign_method(vm, obj, &list->methods, "len", 3, 0, list_length);
}