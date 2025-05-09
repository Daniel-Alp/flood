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
    struct IdentNode **param_names;
    struct TyNode **param_tys;
    struct TyNode *ret_ty;
    struct BlockNode *body;
    u32 arity;
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
    u32 count;
};

struct Parser {
    struct ErrList errlist;
    struct Arena arena;
    struct Node *ast;
    struct Scanner scanner;
    struct Token at;
    struct Token prev;
    bool panic;
};

void init_parser(struct Parser *parser, const char *source);

void release_parser(struct Parser *parser);

void parse(struct Parser *parser);