#pragma once
#include "parse.h"
#define SYM_FLAG_NONE        (0)
#define SYM_FLAG_INITIALIZED (1)
#define SYM_FLAG_LOCAL       (1 << 2)
#define SYM_FLAG_GLOBAL      (1 << 3)
#define SYM_FLAG_IMMUTABLE   (1 << 4)

// TODO add GET_LOCAL_LONG and GET_GLOBAL_LONG opcodes to support more than 256 locals and 256 globals
#define MAX_LOCALS (256)
#define MAX_GLOBALS (256)

struct Symbol {
    struct Span span;
    // NULL means unknown type
    // points to the symbol table arena, or into the type info of an AST node 
    // (e.g. function signatures) which lives on a separate arena
    struct TyNode *ty; 
    u32 flags;
    // if local, index into stack
    // if global, index into global array
    i32 idx;
};

struct SymTable {
    u32 count;
    u32 cap;
    struct Symbol *symbols;
    struct Arena arena;
};

struct Ident {
    struct Span span;
    u32 id;
    u32 depth;
};

struct SemaState {
    // symtable index of enclosing function, if -1 then not inside of function
    i32 enclosing_fn_idx;
    u32 local_count;
    u32 global_count;
    u32 depth;
    struct Ident locals[MAX_LOCALS];
    struct Ident globals[MAX_GLOBALS];
    
    struct ErrList errlist;
    // while traversing AST temporary type info lives on this arena
    struct Arena scratch;
    struct SymTable st;
};

void init_sema_state(struct SemaState *sema);

void release_sema_state(struct SemaState *sema);

void analyze(struct SemaState *sema, struct FileNode *file);