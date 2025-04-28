#pragma once
#include "../util/arena.h"
#include "scan.h"

struct Span {
    const char *start;
    i32 length;    
};

enum NodeKind {
    NODE_UNARY, 
    NODE_BINARY, 
    NODE_LITERAL, 
    NODE_VARIABLE,
    NODE_IF, 
    NODE_BLOCK, 
    NODE_EXPR_STMT, 
    NODE_VAR_DECL
};

enum LitKind {
    LIT_NUMBER,
    LIT_TRUE,
    LIT_FALSE,
};

enum UnOpKind {
    UNOP_NOT, 
    UNOP_NEG
};

enum BinOpKind {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
    BINOP_AND,
    BINOP_OR,
    BINOP_LT,
    BINOP_LEQ,
    BINOP_GT,
    BINOP_GEQ,
    BINOP_EQEQ,
    BINOP_NEQ,
    BINOP_EQ
};

struct UnOp {
    enum UnOpKind kind;
    struct Span span;
};

struct BinOp {
    enum BinOpKind kind;
    struct Span span;
};

struct Node {
    struct Span span;
    enum NodeKind kind;
};

struct LiteralNode {
    struct Node base;
    enum LitKind kind;
};

struct VariableNode {
    struct Node base;
    i32 id;
};

struct UnaryNode {
    struct Node base;
    struct Node *rhs;
    struct UnOp op;
};

struct BinaryNode {
    struct Node base;
    struct Node *lhs;
    struct Node *rhs;
    struct UnOp op;
};

struct NodeLinkedList {
    struct Node *node;
    struct NodeLinkedList *next;
};

struct BlockNode {
    struct Node base;
    struct NodeLinkedList *stmts;
};

struct IfNode {
    struct Node base;
    struct Node *cond;
    struct BlockNode *thn;
    struct BlockNode *els;
};

struct ExprStmtNode {
    struct Node base;
    struct Node *expr;
};

struct VarDeclNode {
    struct Node base;
    struct VariableNode *var;
    struct Node *init;
};

struct Parser {
    struct Scanner scanner;
    struct Token at;
    struct Token prev;
    bool had_error;
    bool panic;
};

struct BlockNode *parse(struct Arena *arena, const char *source);