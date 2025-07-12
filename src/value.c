#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"
#include "object.h"
#include "value.h"
#include "memory.h"

void init_val_array(struct ValArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 8;
    arr->vals = allocate(arr->cap * sizeof(Value));
}

void release_val_array(struct ValArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 0;
    release(arr->vals);
    arr->vals = NULL;
}

u32 push_val_array(struct ValArray *arr, Value val) 
{
    // TODO error if cnt >= 256
    // TODO add LOAD_CONST_LONG opcode
    if (arr->cnt == arr->cap) {
        arr->cap *= 2;
        arr->vals = reallocate(arr->vals, arr->cap * sizeof(Value));
    }
    arr->vals[arr->cnt] = val;
    arr->cnt++;
    return arr->cnt-1;
}

void init_val_table(struct ValTable *tab) 
{
    tab->cnt = 0;
    tab->cap = 8;
    tab->entries = allocate(tab->cap * sizeof(struct ValTableEntry));
    for (i32 i = 0; i < tab->cap; i++)
        tab->entries[i].chars = NULL;
}

void release_val_table(struct ValTable *tab)
{
    // the val table does not own its entries, so we do not free them
    tab->cnt = 0;
    tab->cap = 0;
    release(tab->entries);
    tab->entries = NULL;
}

u32 hash_string(const char *str, u32 len) 
{
    u32 hash = 2166136261u;
    for (int i = 0; i < len; i++) {
        hash ^= (u8)str[i];
        hash *= 16777619;
    }
    return hash;
}

// return pointer to first slot matching key or first empty slot
struct ValTableEntry *get_val_table_slot(
    struct ValTableEntry *vals, 
    u32 cap, 
    u32 hash, 
    u32 len, 
    const char *chars
) {
    u32 i = hash & (cap-1);
    while (true) {
        struct ValTableEntry *entry = vals + i;
        if (entry->chars == NULL
            || (hash == entry->hash && len == entry->len && memcmp(chars, entry->chars, len) == 0))
            return entry;
        i = (i+1) & (cap-1);
    }
}

void insert_val_table(struct ValTable *tab, const char *chars, u32 len, Value val)
{
    if (tab->cnt * TABLE_LOAD_FACTOR >= tab->cap) {
        u32 new_cap = tab->cap * 2;
        struct ValTableEntry *new_entries = allocate(new_cap * sizeof(struct ValTableEntry));
        for (i32 i = 0; i < new_cap; i++) 
            tab->entries[i].chars = NULL;
        for (i32 i = 0; i < tab->cap; i++) {
            struct ValTableEntry entry = tab->entries[i];
            struct ValTableEntry *new_entry 
                = get_val_table_slot(new_entries, new_cap, entry.hash, entry.len, entry.chars);
            memcpy(new_entry, &entry, sizeof(struct ValTableEntry));
        }
        release(tab->entries);
        tab->cap = new_cap;
        tab->entries = new_entries;
    }
    u32 hash = hash_string(chars, len);
    struct ValTableEntry *entry = get_val_table_slot(tab->entries, tab->cap, hash, len, chars);
    entry->hash = hash;
    entry->chars = chars;
    entry->len = len;
    entry->val = val;
    tab->cnt++;
}

bool val_eq(Value val1, Value val2) 
{
    if (val1.tag != val2.tag)
        return false;
    switch (val1.tag) {
        case VAL_BOOL: return AS_BOOL(val1) == AS_BOOL(val2);
        case VAL_NUM: return AS_NUM(val1) == AS_NUM(val2);
        case VAL_NIL: return true;
    }
}

void print_val(Value val) 
{
    switch (val.tag) {
    case VAL_NUM:  printf("%.14g", AS_NUM(val)); break;
    case VAL_BOOL: printf("%s", AS_BOOL(val) ? "true" : "false"); break;
    case VAL_NIL:  printf("null"); break; 
    case VAL_OBJ: 
        if (AS_OBJ(val)->printed) {
            printf("...");
            return;
        } 
        
        AS_OBJ(val)->printed = 1;
        enum ObjTag tag = AS_OBJ(val)->tag;
        switch(tag) {
        case OBJ_FOREIGN_METHOD: {
            const char *name = AS_FOREIGN_METHOD(val)->name;
            printf("<foreign method %s>", name);
            break;
        }
        case OBJ_FN: {
            const char *name = AS_FN(val)->name;
            printf("<function %s>", name);
            break;
        }
        case OBJ_CLOSURE: {
            const char *name = AS_CLOSURE(val)->fn->name;
            printf("<function %s>", name);
            break;
        }
        case OBJ_LIST: {
            printf("[");
            struct ListObj *list = AS_LIST(val);
            Value *vals = list->vals;
            u32 cnt = list->cnt;
            if (cnt > 0) {
                for (i32 i = 0; i < cnt-1; i++) {
                    print_val(vals[i]);
                    printf(", ");
                }
                print_val(vals[cnt-1]);
            }
            printf("]");
            break;
        }
        case OBJ_HEAP_VAL: {
            print_val(AS_HEAP_VAL(val)->val);
            break;
        }
        case OBJ_STRING: {
            printf("%s", AS_STRING(val)->chars);
            break;
        }
        }
        AS_OBJ(val)->printed = 0;
        break;
    }
}