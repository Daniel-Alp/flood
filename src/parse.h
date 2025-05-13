#pragma once
#include "arena.h"
#include "error.h"
#include "scan.h"
#include "ty.h"

enum NodeTag {
    NODE_LITERAL,
    NODE_IDENT,
    NODE_UNARY,
    NODE_BINARY,
    NODE_FN_CALL,
    NODE_VAR_DECL,
    NODE_FN_DECL,
    NODE_EXPR_STMT,
    NODE_BLOCK,
    NODE_IF,
    NODE_RETURN,
    NODE_FILE,
    // TEMP remove when we add functions
    NODE_PRINT
};

struct Node {
    struct Span span;
    enum NodeTag tag;
};

struct LiteralNode {
    struct Node base;
    enum TokenTag lit_tag;
};

struct IdentNode {
    // span is identifier
    struct Node base;
    i32 id;
};

struct UnaryNode {
    // span is op
    struct Node base;
    struct Node *rhs;
    enum TokenTag op_tag;
};

struct BinaryNode {
    // span is op
    struct Node base;
    struct Node *lhs;
    struct Node *rhs;
    enum TokenTag op_tag;
}; 

struct FnCallNode {
    // span is `(`
    struct Node base;
    // the expression that is being called
    struct Node *lhs;
    // span for each argument provided e.g.
    // ```  
    // foo(3+4   , 5, "hello world" )
    //     ^~~ span   ^~~~~~~~~~~~~ span 
    // ```
    struct Span *arg_spans;
    struct Node **args;
    u32 arity;
};

struct VarDeclNode {
    // span is identifier
    struct Node base;
    struct TyNode *ty_hint;
    struct Node *init;
    i32 id;
};

struct FnDeclNode {
    // span is identifier
    struct Node base;
    struct BlockNode *body;
    struct FnTyNode *ty;
    i32 id;
};

// TEMP remove when we add functions
struct PrintNode {
    // span is `print`
    struct Node base;
    struct Node *expr;
};

struct ExprStmtNode {
    // span is unused
    struct Node base;
    struct Node *expr;
};

struct BlockNode {
    // span is `{`
    struct Node base;
    struct Node **stmts;
    u32 count;
};

struct IfNode {
    // span is `if`
    struct Node base;
    struct Node *cond;
    struct BlockNode *thn;
    struct BlockNode *els;
};

struct ReturnNode {
    // span is `return`
    struct Node base;
    struct Node *expr;
};

struct FileNode {
    // span is unused
    struct Node base;
    struct Node **stmts;
    struct FileTyNode *ty;
    u32 count;
};

struct Parser {
    struct ErrList errlist;
    struct Arena arena;
    struct Scanner scanner;
    struct Token at;
    struct Token prev;
    bool panic;
};

// array of every file being compiled
struct FileArr {
    u32 cap;
    u32 count;
    // absolute filepaths
    // malloc'd by realpath
    const char **real_paths;
    // buf[0] = '\0', buf[1] = source[0], buf[length+1] = '\0'
    const char **bufs;
    struct FileNode **files;
};

void init_parser(struct Parser *parser);

void release_parser(struct Parser *parser);

void init_file_array(struct FileArr *arr);

void release_file_array(struct FileArr *arr);

void parse(struct Parser *parser, struct FileArr *arr, const char *path);