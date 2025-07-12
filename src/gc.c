#include <stdlib.h>
#include "gc.h"
#include "object.h"
#include "memory.h"

static inline void push_gray_stack(struct VM *vm, struct Obj *obj)
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

    // mark rest
    // TODO add heap val marking
    while (vm->gray_cnt > 0) {
        struct Obj *obj = vm->gray[vm->gray_cnt-1];
        obj->color = GC_BLACK;
        vm->gray_cnt--;

        switch (obj->tag) {
        case OBJ_FOREIGN_METHOD: {
            struct ForeignMethodObj *f_method = (struct ForeignMethodObj*)obj;
            push_gray_stack(vm, f_method->self);
            break;
        }
        case OBJ_FN: {
            struct FnObj *fn = (struct FnObj*)obj;
            Value *lo = fn->chunk.constants.vals;
            Value *hi = lo + fn->chunk.constants.cnt;
            
            for (Value *ptr = lo; ptr < hi; ptr++) {
                Value val = *ptr;
                if (IS_OBJ(val))
                    push_gray_stack(vm, AS_OBJ(val));
            }
            break;
        }
        case OBJ_CLOSURE: {
            struct ClosureObj *closure = (struct ClosureObj*)obj;
            push_gray_stack(vm, (struct Obj*)closure->fn);
            // LOOKATME this code is very suspicious and probably buggy
            struct HeapValObj **lo = closure->captures;
            struct HeapValObj **hi = lo + closure->capture_cnt;
            for (struct HeapValObj **ptr = lo; ptr < hi; ptr++)
                push_gray_stack(vm, (struct Obj*)(*ptr));
            break;
        }
        case OBJ_LIST: {
            struct ListObj *list = (struct ListObj*)obj;
            Value *val_lo = list->vals;
            Value *val_hi = val_lo + list->cnt;
            for (Value *ptr = val_lo; ptr < val_hi; ptr++) {
                Value val = *ptr;
                if (IS_OBJ(val))
                    push_gray_stack(vm, AS_OBJ(val));
            }
            for (i32 i = 0; i < list->methods.cap; i++) {
                if (list->methods.entries[i].chars)
                    push_gray_stack(vm, AS_OBJ(list->methods.entries[i].val));
            }
            break;
        }
        case OBJ_HEAP_VAL: {
            struct HeapValObj *heap_val = (struct HeapValObj*)obj;
            if (IS_OBJ(heap_val->val))
                push_gray_stack(vm, AS_OBJ(heap_val->val));
        }
        case OBJ_STRING: {
            break;
        }
        }
    }

    // TODO add heap val sweeping
    // sweep (free white objects, reset every color to white)
    struct Obj **indirect = &vm->obj_list;
    struct Obj *obj;
    while ((obj = *indirect) != NULL) {
        if (obj->color == GC_WHITE) {
            *indirect = obj->next;
            // TODO move releases into a separate method
            switch(obj->tag) {
            case OBJ_FOREIGN_METHOD: release_foreign_method_obj((struct ForeignMethodObj*)obj); break;
            case OBJ_FN:             release_fn_obj((struct FnObj*)obj); break;
            case OBJ_CLOSURE:        release_closure_obj((struct ClosureObj*)obj); break;
            case OBJ_LIST:           release_list_obj((struct ListObj*)obj); break;
            case OBJ_HEAP_VAL:       break;
            case OBJ_STRING:         release_string_obj((struct StringObj*)obj); break;
            }
            release(obj);      
        } else {
            obj->color = GC_WHITE;
            indirect = &obj->next;
        }
    }
}