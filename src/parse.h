#pragma once
#include "arena.h"
#include "error.h"
#include "scan.h"

enum NodeTag {
    NODE_ATOM,      // nil, true, false, TODO strings
    NODE_LIST,      // [foo, bar, baz]
    NODE_IDENT,
    NODE_UNARY,
    // +, -, *, /, and, or, [
    // the index operator foo[bar] can be viewed as a high-precedence left-associative binop
    NODE_BINARY,    
    NODE_PROP,      // foo.bar
    NODE_FN_CALL,
    NODE_VAR_DECL,
    NODE_FN_DECL,
    NODE_IMPORT,
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

struct AtomNode {
    struct Node base;
    enum TokenTag atom_tag;
};

struct ListNode {
    // span is `[`
    struct Node base;
    struct Node **items;
    u32 cnt;
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

struct GetPropNode {
    // span is `.`
    struct Node base;
    struct Node *lhs;
    //      foo.bar
    //          ^~~ prop
    struct Span prop;
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
    struct Node *init;
    i32 id;
};

struct FnDeclNode {
    // span is identifier
    struct Node base;
    struct BlockNode *body;
    struct IdentNode *params;
    u32 arity;
    i32 id;
};

// TEMP remove when we add functions
struct PrintNode {
    // span is `print`
    struct Node base;
    struct Node *expr;
};

struct ExprStmtNode {
    // span is `;`
    struct Node base;
    struct Node *expr;
};

struct BlockNode {
    // span is `{`
    struct Node base;
    struct Node **stmts;
    u32 cnt;
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

struct ImportNode {
    // span is identifier
    struct Node base;
    struct Span path;
    i32 id;
};

struct FileNode {
    struct Node base;
    struct Node **stmts;
    u32 cnt;
};

struct Parser {
    struct ErrList errlist;
    struct Arena arena;
    struct Scanner scanner;
    struct Token at;
    struct Token prev;
    bool panic;
};

void init_parser(struct Parser *parser);

void release_parser(struct Parser *parser);

struct FileNode *parse(struct Parser *parser, const char *source);