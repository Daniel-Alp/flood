#include "gc.h"
#include "object.h"

static void push_gray_stack(VM &vm, Obj *const obj)
{
    if (obj->color != GC_WHITE)
        return;
    obj->color = GC_GRAY;
    vm.gray.push(obj);
}

// precondition: all objects white
void collect_garbage(VM &vm)
{
    // TODO
    // // mark roots
    // const Value *const locals_lo = vm.val_stack;
    // const Value *const locals_hi = vm.sp;
    // for (const Value *ptr = locals_lo; ptr < locals_hi; ptr++) {
    //     const Value val = *ptr;
    //     if (IS_OBJ(val))
    //         push_gray_stack(vm, AS_OBJ(val));
    // }
    // // TEMP remove globals when we added user-defined classes
    // // lo != nullptr because vm->globals.cap >= 8
    // const Value *const globals_lo = vm.globals.raw();
    // const Value *const globals_hi = globals_lo + vm.globals.len();
    // for (const Value *ptr = globals_lo; ptr < globals_hi; ptr++) {
    //     const Value val = *ptr;
    //     if (IS_OBJ(val))
    //         push_gray_stack(vm, AS_OBJ(val));
    // }
    // // each call frame contains a closure which points to a function which
    // // contains a constant table whose objects are not in scope but cannot be freed
    // // furthermore, the closure contains an array of pointers to heap vals which also cannot be freed
    // // so we should mark each closure gray
    // const CallFrame *const frame_lo = vm.call_stack;
    // const CallFrame *const frame_hi = frame_lo + vm.call_cnt;
    // for (const CallFrame *frame = frame_lo; frame < frame_hi; frame++)
    //     push_gray_stack(vm, static_cast<Obj *>(frame->closure));

    // while (vm.gray.len() > 0) {
    //     Obj *const obj = vm.gray[vm.gray.len() - 1];
    //     obj->color = GC_BLACK;
    //     vm.gray.pop();

    //     // FIXME!!!
    //     switch (obj->tag) {
    //     case OBJ_FOREIGN_FN: {
    //         break;
    //     }
    //     case OBJ_FN: {
    //         FnObj *const fn = static_cast<FnObj *>(obj);
    //         // lo != nullptr because chunk.constants.cap >= 8
    //         const Value *const lo = fn->chunk.constants().raw();
    //         const Value *const hi = lo + fn->chunk.constants().len();
    //         for (const Value *ptr = lo; ptr < hi; ptr++) {
    //             Value val = *ptr;
    //             if (IS_OBJ(val))
    //                 push_gray_stack(vm, AS_OBJ(val));
    //         }
    //         break;
    //     }
    //     case OBJ_HEAP_VAL: {
    //         HeapValObj *const heap_val = static_cast<HeapValObj *>(obj);
    //         if (IS_OBJ(heap_val->val))
    //             push_gray_stack(vm, AS_OBJ(heap_val->val));
    //         break;
    //     }
    //     case OBJ_CLOSURE: {
    //         ClosureObj *const closure = static_cast<ClosureObj *>(obj);
    //         push_gray_stack(vm, static_cast<Obj *>(closure->fn));
    //         if (closure->capture_cnt == 0)
    //             break;
    //         HeapValObj *const *const lo = closure->captures;
    //         HeapValObj *const *const hi = lo + closure->capture_cnt;
    //         for (HeapValObj *const *ptr = lo; ptr < hi; ptr++)
    //             push_gray_stack(vm, static_cast<Obj *>(*ptr));
    //         break;
    //     }
    //     case OBJ_LIST: {
    //         ListObj *const list = static_cast<ListObj *>(obj);
    //         // lo != nullptr because list.cap >= 8
    //         const Value *const val_lo = list->vals.raw();
    //         const Value *const val_hi = val_lo + list->vals.len();
    //         for (const Value *ptr = val_lo; ptr < val_hi; ptr++) {
    //             Value val = *ptr;
    //             if (IS_OBJ(val))
    //                 push_gray_stack(vm, AS_OBJ(val));
    //         }
    //         break;
    //     }
    //     case OBJ_STRING: {
    //         break;
    //     }
    //     }
    // }

    // // sweep (free white objects, reset every color to white)
    // Obj **indirect = &vm.obj_list;
    // Obj *obj;
    // while ((obj = *indirect) != nullptr) {
    //     if (obj->color == GC_WHITE) {
    //         *indirect = obj->next;
    //         delete obj;
    //     } else {
    //         obj->color = GC_WHITE;
    //         indirect = &obj->next;
    //     }
    // }
}
