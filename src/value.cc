// #include "value.h"
// #include "dynarr.h"

// static Assoc &find(Slice<Assoc>& slice, const Slice<char> key)
// {
//     i32 i = key.hash() & (slice.len()-1);
//     while (true) {
//         if (key == slice[i].key)
//             return slice[i];
//         i = (i+1) & slice.len()-1;
//     }
// }

// void ValTable::insert(String &&key, Value val)
// {
//     if (cnt >= cap * TABLE_LOAD_FACTOR) {
//         const i32 new_cap = cap * 2;
//     }
// }

// // void ValTable::insert(String &&key, Value val)
// // {
// //     if (arr.size() >= arr.cap() * TABLE_LOAD_FACTOR) {
// //         const i32 new_cap = arr.cap() * 2;
// //         Dynarr<Assoc> new_arr;
// //         // FIXME using vector doesn't make sense I think... 
// //         // my old implementation did actually make sense!   
// //         for (i32 i = 0; i < arr.cap(); i++) {
// //             Assoc &assoc = find(new_arr, arr[i].key);
// //             if (assoc.key.size() == 0) 
// //                 continue;
// //             assoc = move(arr[i]);
// //         }
// //         arr = move(new_arr);
// //     }
// //     Assoc &assoc = find(arr, key);
// //     assoc.key = key;
// //     assoc.val = val;
// // }

// // // TODO have this take an ObjString*
// // // return pointer to first slot matching key or first empty slot
// // struct ValTableEntry *get_val_table_slot(
// //     struct ValTableEntry *vals, 
// //     i32 cap, 
// //     u32 hash, 
// //     i32 len, 
// //     const char *chars
// // ) {
// //     i32 i = hash & (cap-1);
// //     while (true) {
// //         struct ValTableEntry *entry = vals + i;
// //         if (entry->chars == NULL
// //             || (hash == entry->hash && len == entry->len && memcmp(chars, entry->chars, len) == 0))
// //             return entry;
// //         i = (i+1) & (cap-1);
// //     }
// // }

// // // TODO make this take an StringObj probably
// // void insert_val_table(struct ValTable *tab, const char *chars, i32 len, Value val)
// // {
// //     // TODO consider passing in the hash here so that we don't recompute it when we don't need to
// //     if (tab->cnt >= tab->cap * TABLE_LOAD_FACTOR) {
// //         i32 new_cap = tab->cap * 2;
// //         struct ValTableEntry *new_entries = allocate(new_cap * sizeof(struct ValTableEntry));
// //         memset(new_entries, 0, new_cap * sizeof(struct ValTableEntry));
// //         for (i32 i = 0; i < tab->cap; i++) {
// //             struct ValTableEntry entry = tab->entries[i];
// //             if (entry.chars == NULL)
// //                 continue;
// //             struct ValTableEntry *new_entry = get_val_table_slot(new_entries, new_cap, entry.hash, entry.len, entry.chars);
// //             memcpy(new_entry, &entry, sizeof(struct ValTableEntry));
// //         }
// //         release(tab->entries);
// //         tab->cap = new_cap;
// //         tab->entries = new_entries;
// //     }
// //     u32 hash = hash_string(chars, len);
// //     struct ValTableEntry *entry = get_val_table_slot(tab->entries, tab->cap, hash, len, chars);
// //     entry->hash = hash;
// //     entry->chars = chars;
// //     entry->len = len;
// //     entry->val = val;
// //     tab->cnt++;
// // }

// bool val_eq(const Value val1, const Value val2) 
// {
//     if (val1.tag != val2.tag)
//         return false;
//     switch (val1.tag) {
//     case VAL_BOOL: return AS_BOOL(val1) == AS_BOOL(val2);
//     case VAL_NUM:  return AS_NUM(val1) == AS_NUM(val2);
//     case VAL_NULL: return true;
//     case VAL_OBJ:  return AS_OBJ(val1) == AS_OBJ(val2); // TODO proper string comparison (?)
//     }
//     // TODO should never reach this case
// }

// // void print_val(Value val) 
// // {
// //     switch (val.tag) {
// //     case VAL_NUM:  printf("%.14g", AS_NUM(val)); break;
// //     case VAL_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
// //     case VAL_NULL: printf("null"); break; 
// //     case VAL_OBJ: 
// //         if (AS_OBJ(val)->printed) {
// //             printf("...");
// //             return;
// //         } 
// //         AS_OBJ(val)->printed = 1;
// //         enum ObjTag tag = AS_OBJ(val)->tag;
// //         switch(tag) {
// //         case OBJ_FOREIGN_FN: {
// //             const char *name = AS_FOREIGN_FN(val)->name->chars;
// //             printf("<foreign function %s>", name);
// //             break;   
// //         }
// //         case OBJ_FOREIGN_METHOD: {
// //             // FIXME this is doing print_null
// //             const char *name = AS_FOREIGN_METHOD(val)->fn->name->chars;
// //             printf("<foreign method %s>", name);
// //             break;
// //         }
// //         case OBJ_FN: {
// //             const char *name = AS_FN(val)->name->chars;
// //             printf("<function %s>", name);
// //             break;
// //         }
// //         case OBJ_CLOSURE: {
// //             const char *name = AS_CLOSURE(val)->fn->name->chars;
// //             printf("<closure %s>", name);
// //             break;
// //         }
// //         case OBJ_CLASS: {
// //             const char *name = AS_CLASS(val)->name->chars;
// //             printf("<class %s>", name);
// //             break;
// //         }
// //         case OBJ_INSTANCE: {
// //             const char *name = AS_OBJ(val)->klass->name->chars;
// //             printf("<instance %s>", name);
// //             break;
// //         }
// //         case OBJ_METHOD: {
// //             const char *name = AS_METHOD(val)->closure->fn->name->chars;
// //             printf("<method %s>", name);
// //             break;
// //         }
// //         case OBJ_LIST: {
// //             printf("[");
// //             struct ListObj *list = AS_LIST(val);
// //             Value *vals = list->vals;
// //             i32 cnt = list->cnt;
// //             if (cnt > 0) {
// //                 for (i32 i = 0; i < cnt-1; i++) {
// //                     print_val(vals[i]);
// //                     printf(", ");
// //                 }
// //                 print_val(vals[cnt-1]);
// //             }
// //             printf("]");
// //             break;
// //         }
// //         case OBJ_HEAP_VAL: {
// //             print_val(AS_HEAP_VAL(val)->val);
// //             break;
// //         }
// //         case OBJ_STRING: {
// //             printf("%s", AS_STRING(val)->chars);
// //             break;
// //         }
// //         }
// //         AS_OBJ(val)->printed = 0;
// //         break;
// //     }
// // }
