#pragma once
#include "../util/arena.h"
#include "scan.h"

struct Span {
    const char *start;
    i32 length;    
};

enum NodeType {
    NODE_UNARY, 
    NODE_BINARY, 
    NODE_LITERAL, 
    NODE_VARIABLE,
    NODE_IF, 
    NODE_BLOCK, 
    NODE_EXPR_STMT, 
    NODE_VAR_DECL
};

struct Node {
    enum NodeType tag;
};

struct LiteralNode {
    struct Node base;
    struct Token val;
};

struct VariableNode {
    struct Node base;
    struct Token name;
    i32 id;
};

struct UnaryNode {
    struct Node base;
    struct Node *rhs;
    struct Token op;
};

struct BinaryNode {
    struct Node base;
    struct Node *lhs;
    struct Node *rhs;
    struct Token op;
};

struct NodeList {
    struct Node *node;
    struct NodeList *next;
};

struct BlockNode {
    struct Node base;
    struct NodeList *stmts;
};

struct IfNode {
    struct Node base;
    struct Span if_span;
    struct Span cond_span;
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