#pragma once
#include "arena.h"
#include "chunk.h"
#include "symbol.h"
#define MAX_CALL_FRAMES (64)
#define MAX_STACK       (MAX_CALL_FRAMES * 256)

struct CallFrame {
    struct FnObj *fn;
    u8 *ip;
    Value *bp;
};

struct FileArray {
    u32 cap;
    u32 cnt;
    const char **paths;      
    struct SymMap *sym_maps;
};

enum InterpResult {
    INTERP_COMPILE_ERR,
    INTERP_RUNTIME_ERR,
    INTERP_OK
};

struct VM {
    struct CallFrame call_stack[MAX_CALL_FRAMES];
    u16 call_count;

    Value val_stack[MAX_STACK];
    // stack pointer is stored in the run_vm function

    struct ValArray globals;
    
    // WIP!
    // Explanation of imports, calling flood from C
    //
    // Suppose we had the following project structure
    // ```
    //      src/
    //          dirA/
    //              fileA.fl
    //          dirB/
    //              fileB.fl
    //      fileC.fl
    // ```
    // If we wanted to use fileB in fileA, we would do 
    // ```
    //      import "../dirB/fileB.fl" as fileB
    // ```  
    // If we wanted to use fileB in fileC, we would do
    // ```
    //      import "dirB/fileB.fl"
    // ```
    // What if we want to invoke a function in fileB from C?
    // When creating a VM we define `source_dir` to be the path to the top directory of the project 
    // (in the example above it's the path to the `src/` directory). 
    // Then to invoke a function we pass the path to the file (starting from `source_dir`) and the function name. 
    // 
    // To find the function definition, the VM looks at its `files` field, which associates 
    // the real path of the file to a SymMap, which maps symbol name to information about the symbol.
    struct FileArray files;
    const char *source_dir; // CURRENTLY UNUSED
    
    // linked list of objects
    struct Obj *obj_list;

    // symbol spans live on this arena 
    // TODO move other allocs to this arena potentially
    struct Arena arena;
};

void init_vm(struct VM *vm, const char *source_dir);

void release_vm(struct VM *vm);

void push_file_array(struct FileArray *files, const char *name, struct SymMap map);

enum InterpResult run_vm(struct VM *vm, struct FnObj *fn);

struct Obj *alloc_vm_obj(struct VM *vm, u64 size);