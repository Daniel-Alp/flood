#pragma once
#include "chunk.h"

// These are some notes/plans
// compilation:
//      - recursively parse file and its imports
//      - for every file create a list of public functions and variables, used in the semantic phase
//      - typecheck
//      - compile to bytecode 
// 
// runtime:
// the VM state consists of
//      - call stack
//          - each stack frame contains a pointer to the value stack and the FnObj being executed
//      - array of FileObj
// 
// handling imports:
//      import "fileA.fl" as fileA
//      fileA.x;   
// compiles to
//      OP_FILEOBJ
//      <index into FileObj array>
//      OP_GET_FILEOBJ
//      <index into global arr of fileA for x>
// if a file has not been imported this will 
//      - push a FileObj onto the value stack, 
//      - push the FileObj's script onto the call stack 
// 
// handling locals:
//      x;
// compiles to
//      OP_GET_LOCAL
//      <index into value stack>
// 
// handling globals:
//      x;
// compiles to
//      OP_GET_GLOBAL
//      <index into globals array of the FileObj the FnObj points to>

enum ObjTag {
    OBJ_FN,
    OBJ_FILE,
};

struct Obj {
    enum ObjTag tag;
};

// runtime representation of a function literal
struct FnObj {
    struct Obj base;
    const char *name;
    struct File *file;
    struct Chunk chunk;
    u32 arity;
};

// runtime represenation of a file
struct FileObj {
    struct Obj base;
    struct ValArray globals;
    struct FnObj script;
    bool executed;
};