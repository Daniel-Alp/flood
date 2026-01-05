// #include <string.h>
// #include <stdlib.h>
// #include "object.h"
// #include "gc.h"
// #include "memory.h"

// // TODO proper comments



// struct Obj *alloc_vm_obj(struct VM *vm, u64 size)
// {
//     struct Obj *obj = allocate(size);
//     obj->klass = NULL;
//     obj->next = vm->obj_list;
//     obj->color = GC_WHITE;
//     obj->printed = 0;
//     vm->obj_list = obj;
//     return obj;
// }

// void release_obj(struct Obj *obj)
// {
//     switch(obj->tag) {
//     case OBJ_FOREIGN_METHOD: release_foreign_method_obj((struct ForeignMethodObj*)obj); break;
//     case OBJ_FOREIGN_FN:     release_foreign_fn_obj((struct ForeignFnObj*)obj); break;
//     case OBJ_FN:             release_fn_obj((struct FnObj*)obj); break;
//     case OBJ_HEAP_VAL:       break;
//     case OBJ_CLOSURE:        release_closure_obj((struct ClosureObj*)obj); break;
//     case OBJ_CLASS:          release_class_obj((struct ClassObj*)obj); break;
//     case OBJ_INSTANCE:       release_instance_obj((struct InstanceObj*)obj); break;
//     case OBJ_METHOD:         release_method_obj((struct MethodObj*)obj); break;
//     case OBJ_LIST:           release_list_obj((struct ListObj*)obj); break;
//     case OBJ_STRING:         release_string_obj((struct StringObj*)obj); break;
//     }
// }

// void init_foreign_fn_obj(
//     struct ForeignFnObj *f_fn,
//     struct StringObj *name,
//     i32 arity,
//     ForeignFn code
// ) {
//     f_fn->base.tag = OBJ_FOREIGN_FN;
//     f_fn->name = name;
//     f_fn->code = code;
//     f_fn->arity = arity;
// }

// void release_foreign_fn_obj(struct ForeignFnObj *f_fn) {
//     // GC handles lifetime of name
//     f_fn->name = NULL;
//     f_fn->arity = 0;
// }

// void init_foreign_method_obj(
//     struct ForeignMethodObj *f_method, 
//     struct ForeignFnObj *f_fn,
//     struct Obj* self
// ) {
//     f_method->base.tag = OBJ_FOREIGN_METHOD;
//     f_method->self = self;
//     f_method->fn = f_fn;
// }

// void release_foreign_method_obj(struct ForeignMethodObj *f_method)
// {
//     // GC handles lifetime of fn
//     f_method->fn = NULL;
//     f_method->self = NULL;
// }

// void init_fn_obj(struct FnObj *fn, struct StringObj *name, i32 arity) 
// {
//     fn->base.tag = OBJ_FN;
//     init_chunk(&fn->chunk);
//     fn->name = name;
//     fn->arity = arity;
// }

// void release_fn_obj(struct FnObj *fn) 
// {
//     release_chunk(&fn->chunk);
//     // GC handles lifetime of name
//     fn->name = NULL;
//     fn->arity = 0;
// }

// void init_heap_val_obj(struct HeapValObj *heap_val, Value val)
// {
//     heap_val->base.tag = OBJ_HEAP_VAL;
//     heap_val->val = val;
// }

// // NOTE: 
// // init_closure_obj only creates fn and allocates memory for the heap vals array
// // in the run_vm function the heap vals ptrs are set
// void init_closure_obj(struct ClosureObj *closure, struct FnObj *fn, u8 n) 
// {
//     closure->base.tag = OBJ_CLOSURE;
//     closure->fn = fn;
//     closure->capture_cnt = n;
//     closure->captures = allocate(n * sizeof(struct HeapValObj*));
// }

// void release_closure_obj(struct ClosureObj *closure)
// {
//     // the closure does not own the function
//     // the closure also does not own the heap vals it has references to
//     release(closure->captures);
//     closure->capture_cnt = 0;
//     closure->captures = NULL;
// }

// void init_class_obj(struct ClassObj *class, struct StringObj *name, struct VM *vm)
// {
//     class->base.tag = OBJ_CLASS;
//     class->base.klass = vm->class_class;
//     class->name = name;
//     init_val_table(&class->methods);
// }

// void release_class_obj(struct ClassObj *class)
// {
//     // GC handles lifetime of name
//     release_val_table(&class->methods);
// }

// // NOTE: vals is the methods defined on the class
// void init_instance_obj(struct InstanceObj *instance, struct ClassObj *klass)
// {
//     instance->base.tag = OBJ_INSTANCE;
//     instance->base.klass = klass;
//     init_val_table(&instance->fields);
// }

// void release_instance_obj(struct InstanceObj *instance)
// {
//     release_val_table(&instance->fields);
// }

// // TODO consider copying contents of closure to avoid going through pointer
// void init_method_obj(struct MethodObj *method, struct ClosureObj *closure, struct InstanceObj *self)
// {
//     method->base.tag = OBJ_METHOD;
//     method->closure = closure;
//     method->self = self;
// }

// // TODO consider copying contents of closure to avoid going through pointer
// void release_method_obj(struct MethodObj *method)
// {
//     method->closure = NULL;
//     method->self = NULL;
// }

// // NOTE: vals points to the start of the list's elements in the stack 
// void init_list_obj(struct ListObj *list, Value *vals, i32 cnt, struct VM *vm)
// {
//     list->base.tag = OBJ_LIST;
//     list->base.klass = vm->list_class;
//     i32 cap = 8;
//     while (cnt > cap)
//         cap *= 2;
//     list->cnt = cnt;
//     list->cap = cap;
//     list->vals = allocate(sizeof(Value) * cap);
//     for (i32 i = 0; i < cnt; i++)
//         list->vals[i] = vals[i];
// }

// void release_list_obj(struct ListObj *list)
// {
//     list->cnt = 0;
//     list->cap = 0;
//     release(list->vals);
//     list->vals = NULL;
// }

// void init_string_obj(struct StringObj *str, u32 hash, i32 len, char *chars, struct VM *vm)
// {
//     str->base.tag = OBJ_STRING;
//     str->base.klass = vm->string_class;
//     str->hash = hash;
//     str->len = len;
//     str->chars = chars;
// }

// void release_string_obj(struct StringObj *str)
// {
//     str->hash = 0;
//     str->len = 0;
//     release(str->chars);
//     str->chars = NULL;
// }

// struct StringObj *string_from_span(struct VM *vm, struct Span span)
// {
//     char *chars = allocate((span.len+1)*sizeof(char));
//     memcpy(chars, span.start, span.len);
//     chars[span.len] = '\0';
//     struct StringObj *str = (struct StringObj*)alloc_vm_obj(vm, sizeof(struct StringObj));
//     init_string_obj(str, hash_string(chars, span.len), span.len, chars, vm);
//     return str;
// }

// struct StringObj *string_from_c_str(struct VM *vm, const char *c_str)
// {
//     i32 len = strlen(c_str);
//     char *chars = allocate((len+1)*sizeof(char));
//     strcpy(chars, c_str);
//     struct StringObj *str = (struct StringObj*)alloc_vm_obj(vm, sizeof(struct StringObj));
//     init_string_obj(str, hash_string(chars, len), len, chars, vm);
//     return str;
// }
