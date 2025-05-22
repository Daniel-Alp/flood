#pragma once
#include "symbol.h"
#include "parse.h"
// TODO add GET_LOCAL_LONG and GET_GLOBAL_LONG opcodes to support more than 256 locals and 256 globals
#define MAX_LOCALS (256)
#define MAX_GLOBALS (256)

struct Ident {
    struct Span span;
    u32 id;
    u32 depth;
};

struct SemaState {
    u32 local_cnt;
    u32 global_cnt;
    u32 depth;
    struct Ident locals[MAX_LOCALS];
    struct Ident globals[MAX_GLOBALS];
    
    struct ErrList errlist;
    struct SymArr *sym_arr;
};

void init_sema_state(struct SemaState *sema, struct SymArr *sym_arr);

void release_sema_state(struct SemaState *sema);

void analyze(struct SemaState *sema, struct FileNode *file);