#include <assert.h>
#include <stdlib.h>
#include "gc.h"
#include "object.h"
#include "memory.h"

static void push_gray_stack(struct VM *vm, struct Obj *obj)
{
    if (obj->color != GC_WHITE)
        return;
    if (vm->gray_cnt == vm->gray_cap) {
        vm->gray_cap *= 2;
        vm->gray = reallocate(vm->gray, vm->gray_cap * sizeof(struct Obj*));
    }
    vm->gray[vm->gray_cnt] = obj;
    vm->gray_cnt++;
    obj->color = GC_GRAY;
}

// TODO move other mark methods into separate fns
static void mark_table(struct VM *vm, struct ValTable *tab)
{
    for (i32 i = 0; i < tab->cap; i++) {
        struct ValTableEntry entry = tab->entries[i];
        if (entry.chars != NULL && IS_OBJ(entry.val))
            push_gray_stack(vm, AS_OBJ(entry.val));
    }
}

// precondition: all objects white
void collect_garbage(struct VM *vm) 
{
    // mark roots
    Value *locals_lo = vm->val_stack;
    Value *locals_hi = vm->sp;
    for (Value *ptr = locals_lo; ptr < locals_hi; ptr++) {
        Value val = *ptr;
        if (IS_OBJ(val))
            push_gray_stack(vm, AS_OBJ(val));
    }
    // TEMP remove globals when we added user-defined classes
    // lo != NULL because vm->globals.cap >= 8
    Value *globals_lo = vm->globals.vals;
    Value *globals_hi = globals_lo + vm->globals.cnt;
    for (Value *ptr = globals_lo; ptr < globals_hi; ptr++) {
        Value val = *ptr;
        if (IS_OBJ(val))
            push_gray_stack(vm, AS_OBJ(val));
    }    
    // each call frame contains a closure which points to a function which
    // contains a constant table whose objects are not in scope but cannot be freed
    // furthermore, the closure contains an array of pointers to heap vals which also cannot be freed
    // so we should mark each closure gray
    struct CallFrame *frame_lo = vm->call_stack;
    struct CallFrame *frame_hi = frame_lo + vm->call_cnt;
    for (struct CallFrame *frame = frame_lo; frame < frame_hi; frame++)
        push_gray_stack(vm, (struct Obj*)frame->closure);

    push_gray_stack(vm, (struct Obj*)vm->list_class);
    push_gray_stack(vm, (struct Obj*)vm->string_class);
    push_gray_stack(vm, (struct Obj*)vm->class_class);

    while (vm->gray_cnt > 0) {
        struct Obj *obj = vm->gray[vm->gray_cnt-1];
        obj->color = GC_BLACK;
        vm->gray_cnt--;

        switch (obj->tag) {
        case OBJ_FOREIGN_FN: {
            struct ForeignFnObj *f_fn = (struct ForeignFnObj*)obj;
            push_gray_stack(vm, (struct Obj*)f_fn->name);
            break;
        }
        case OBJ_FOREIGN_METHOD: {
            struct ForeignMethodObj *f_method = (struct ForeignMethodObj*)obj;
            push_gray_stack(vm, f_method->self);
            push_gray_stack(vm, (struct Obj*)f_method->fn);
            break;
        }
        case OBJ_FN: {
            struct FnObj *fn = (struct FnObj*)obj;
            push_gray_stack(vm, (struct Obj*)fn->name);
            // lo != NULL because chunk.constants.cap >= 8
            Value *lo = fn->chunk.constants.vals;
            Value *hi = lo + fn->chunk.constants.cnt;
            for (Value *ptr = lo; ptr < hi; ptr++) {
                Value val = *ptr;
                if (IS_OBJ(val))
                    push_gray_stack(vm, AS_OBJ(val));
            }
            break;
        }
        case OBJ_HEAP_VAL: {
            struct HeapValObj *heap_val = (struct HeapValObj*)obj;
            if (IS_OBJ(heap_val->val))
                push_gray_stack(vm, AS_OBJ(heap_val->val));
            break;
        }
        case OBJ_CLOSURE: {
            struct ClosureObj *closure = (struct ClosureObj*)obj;
            push_gray_stack(vm, (struct Obj*)closure->fn);
            if (closure->capture_cnt == 0)
                break; 
            struct HeapValObj **lo = closure->captures;
            struct HeapValObj **hi = lo + closure->capture_cnt;
            for (struct HeapValObj **ptr = lo; ptr < hi; ptr++)
                push_gray_stack(vm, (struct Obj*)(*ptr));
            break;
        }
        case OBJ_CLASS: {
            struct ClassObj *class = (struct ClassObj*)obj;
            push_gray_stack(vm, (struct Obj*)class->name);
            mark_table(vm, &class->methods);
            break;
        }
        case OBJ_INSTANCE: {
            struct InstanceObj *instance = (struct InstanceObj*)obj;
            push_gray_stack(vm, (struct Obj*)instance->base.class);
            mark_table(vm, &instance->fields);
            break;
        }
        case OBJ_METHOD: {
            struct MethodObj *method = (struct MethodObj*)obj;
            push_gray_stack(vm, (struct Obj*)method->closure);
            push_gray_stack(vm, (struct Obj*)method->self);
            break;
        }
        case OBJ_LIST: {
            struct ListObj *list = (struct ListObj*)obj;
            // lo != NULL because list.cap >= 8
            Value *val_lo = list->vals;
            Value *val_hi = val_lo + list->cnt;
            for (Value *ptr = val_lo; ptr < val_hi; ptr++) {
                Value val = *ptr;
                if (IS_OBJ(val))
                    push_gray_stack(vm, AS_OBJ(val));
            }
            break;
        }
        case OBJ_STRING: {
            break;
        }
        }
    }

    // sweep (free white objects, reset every color to white)
    struct Obj **indirect = &vm->obj_list;
    struct Obj *obj;
    while ((obj = *indirect) != NULL) {
        if (obj->color == GC_WHITE) {
            *indirect = obj->next;
            release_obj(obj);    
            release(obj);
        } else {
            obj->color = GC_WHITE;
            indirect = &obj->next;
        }
    }
}