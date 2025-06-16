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
    Value *globals_lo = vm->globals.vals;
    Value *globals_hi = globals_lo + vm->globals.cnt;
    for (Value *ptr = globals_lo; ptr < globals_hi; ptr++) {
        Value val = *ptr;
        if (IS_OBJ(val))
            push_gray_stack(vm, AS_OBJ(val));
    }    
    // each call frame contains a fn which has a constant table
    // this table may contain objects such as others fns and strings
    // so we should mark each fn gray
    struct CallFrame *frame_lo = vm->call_stack;
    struct CallFrame *frame_hi = frame_lo + vm->call_cnt;
    for (struct CallFrame *frame = frame_lo; frame < frame_hi; frame++)
        push_gray_stack(vm, (struct Obj*)frame->fn);

    // mark rest
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
            Value *val_lo = fn->chunk.constants.vals;
            Value *val_hi = val_lo + fn->chunk.constants.cnt;
            
            for (Value *ptr = val_lo; ptr < val_hi; ptr++) {
                Value val = *ptr;
                if (IS_OBJ(val))
                    push_gray_stack(vm, AS_OBJ(val));
            }
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
            // TODO move releases into a separate method
            switch(obj->tag) {
            case OBJ_FOREIGN_METHOD: release_foreign_method_obj((struct ForeignMethodObj*)obj); break;
            case OBJ_FN:             release_fn_obj((struct FnObj*)obj); break;
            case OBJ_LIST:           release_list_obj((struct ListObj*)obj); break;
            case OBJ_STRING:         release_string_obj((struct StringObj*)obj); break;
            }
            release(obj);      
        } else {
            obj->color = GC_WHITE;
            indirect = &obj->next;
        }
    }
}