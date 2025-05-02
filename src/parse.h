#pragma once
#include "arena.h"
#include "scan.h"

enum TyTag {
    TY_NUM,
    TY_BOOL,
};

struct TyNode {
    struct Span span;
    enum TyTag tag;
};

enum NodeTag {
    NODE_LITERAL,
    NODE_IDENT,
    NODE_UNARY,
    NODE_BINARY,
    NODE_VAR_DECL,
    NODE_EXPR_STMT,
    NODE_BLOCK,
    NODE_IF,
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
    struct Node base;
    i32 id;
};

struct UnaryNode {
    struct Node base;
    struct Node *rhs;
    enum TokenTag op_tag;
};

struct BinaryNode {
    struct Node base;
    struct Node *lhs;
    struct Node *rhs;
    enum TokenTag op_tag;
}; 

struct VarDeclNode {
    struct Node base;
    struct TyNode *ty_hint;
    struct Node *init;
    i32 id;
};

struct ExprStmtNode {
    struct Node base;
    struct Node *expr;
};

struct BlockNode {
    struct Node base;
    struct Node **stmts;
    u32 count;
};

struct IfNode {
    struct Node base;
    struct Node *cond;
    struct BlockNode *thn;
    struct BlockNode *els;
};

struct ParseErr {
    struct Span span;
    const char *msg;
};

struct ParseErrList {
    u32 count;
    u32 cap;
    struct ParseErr *errs;
};

struct Parser {
    struct ParseErrList errlist;
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