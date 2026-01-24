#pragma once
#include "scan.h"
#define MAX_LOCALS (256)

enum NodeTag {
    NODE_ATOM,
    NODE_LIST,
    NODE_IDENT,
    NODE_UNARY,
    NODE_BINARY,
    NODE_SELECTOR,
    NODE_SUBSCR,
    NODE_ASSIGN,
    NODE_CALL,
    NODE_VAR_DECL,
    NODE_FN_DECL,
    NODE_CLASS_DECL,
    NODE_EXPR_STMT,
    NODE_BLOCK,
    NODE_IF,
    NODE_RETURN,
    // TEMP remove when we add functions
    NODE_PRINT,
    NODE_IMPORT,
    NODE_MODULE
};

// TODO replace this shit with slices where needed

struct BlockNode;

struct Node {
    const Span span;
    const NodeTag tag;
    Node(Span span, NodeTag tag) : span(span), tag(tag) {}
};    

struct AtomNode : public Node {
    const TokenTag atom_tag;
    AtomNode(const Span span, const TokenTag atom_tag) : Node(span, NODE_ATOM), atom_tag(atom_tag) {}
};

struct ListNode : public Node {
    // span is `[`
    Node *const *const items;
    const i32 cnt;
    ListNode(const Span span, Node *const *const items, const i32 cnt) : Node(span, NODE_LIST), items(items), cnt(cnt)
    {
    }
};

struct IdentNode : public Node {
    // span is identifier
    i32 id;
    IdentNode(const Span span) : Node(span, NODE_IDENT), id(-1) {}
};

struct UnaryNode : public Node {
    // span is op
    Node *const rhs;
    const TokenTag op_tag;
    UnaryNode(const Span span, Node *const rhs, const TokenTag op_tag)
        : Node(span, NODE_UNARY), rhs(rhs), op_tag(op_tag)
    {
    }
};

struct BinaryNode : public Node {
    // span is op
    Node *const lhs;
    Node *const rhs;
    const TokenTag op_tag;
    BinaryNode(const Span span, Node *const lhs, Node *const rhs, const TokenTag op_tag)
        : Node(span, NODE_BINARY), lhs(lhs), rhs(rhs), op_tag(op_tag)
    {
    }
};

struct SelectorNode : public Node {
    // span is `.` or `:`
    Node *const lhs;
    //      foo.bar
    //          ^~~ sym
    const Span sym;
    const TokenTag op_tag;
    SelectorNode(const Span span, Node *const lhs, const Span sym, const TokenTag op_tag)
        : Node(span, NODE_SELECTOR), lhs(lhs), sym(sym), op_tag(op_tag)
    {
    }
};

struct SubscrNode : public Node {
    // span is `[`
    Node *const lhs;
    Node *const rhs;
    SubscrNode(const Span span, Node *const lhs, Node *const rhs) : Node(span, NODE_SUBSCR), lhs(lhs), rhs(rhs) {}
};

struct AssignNode : public Node {
    // span is `=`, `+=`, `-=`, `*=`, `/=`, `//=`, `%=`
    Node *const lhs;
    Node *const rhs;
    const TokenTag op_tag;
    AssignNode(const Span span, Node *const lhs, Node *const rhs, const TokenTag op_tag)
        : Node(span, NODE_ASSIGN), lhs(lhs), rhs(rhs), op_tag(op_tag)
    {
    }
};

struct CallNode : public Node {
    // span is `(`
    Node *const lhs;
    Node *const *const args;
    const i32 arity;
    CallNode(const Span span, Node *const lhs, Node *const *const args, const i32 arity)
        : Node(span, NODE_CALL), lhs(lhs), args(args), arity(arity)
    {
    }
};

struct VarDeclNode : public Node {
    // span is identifier
    Node *const init;
    i32 id;
    VarDeclNode(const Span span, Node *const init) : Node(span, NODE_VAR_DECL), init(init), id(-1) {}
};

struct FnDeclNode : public Node {
    // span is identifier
    BlockNode *const body;
    IdentNode *const params;
    const i32 arity;
    i32 id;
    // NOTE:
    // when a closure is created at runtime there are two ways to get a ptr to a captured value
    //      (1) the ptr is in the current stack frame
    //      (2) the ptr is in the parent closure's ptr list
    // in both cases the ptr is copied to the new closure's ptr list
    i32 stack_capture_cnt;
    i32 stack_captures[MAX_LOCALS];
    i32 parent_capture_cnt;
    i32 parent_captures[MAX_LOCALS];
    FnDeclNode *parent;
    FnDeclNode(const Span span, BlockNode *const body, IdentNode *const params, const i32 arity)
        : Node(span, NODE_FN_DECL)
        , body(body)
        , params(params)
        , arity(arity)
        , id(-1)
        , stack_capture_cnt(0)
        , parent_capture_cnt(0)
        , parent(nullptr)
    {
    }
};

struct ClassDeclNode : public Node {
    // span is identifier
    FnDeclNode *const *const methods;
    const i32 cnt;
    i32 id;
    ClassDeclNode(const Span span, FnDeclNode *const *const methods, const i32 cnt)
        : Node(span, NODE_CLASS_DECL), methods(methods), cnt(cnt)
    {
    }
};

// TEMP remove when we add functions
struct PrintNode : public Node {
    // span is `print`
    Node *const expr;
    PrintNode(const Span span, Node *const expr) : Node(span, NODE_PRINT), expr(expr) {}
};

struct ExprStmtNode : public Node {
    // span is `;`
    Node *const expr;
    ExprStmtNode(const Span span, Node *const expr) : Node(span, NODE_EXPR_STMT), expr(expr) {}
};

struct BlockNode : public Node {
    // span is `{`
    Node *const *const stmts;
    const i32 cnt;
    i32 local_cnt; // locals declared in block, including params if block is fn body
    BlockNode(const Span span, Node *const *const stmts, const i32 cnt)
        : Node(span, NODE_BLOCK), stmts(stmts), cnt(cnt), local_cnt(0)
    {
    }
};

struct IfNode : public Node {
    // span is `if`
    Node *const cond;
    BlockNode *const thn;
    BlockNode *const els;
    IfNode(const Span span, Node *const cond, BlockNode *const thn, BlockNode *const els)
        : Node(span, NODE_IF), cond(cond), thn(thn), els(els)
    {
    }
};

struct ReturnNode : public Node {
    // span is `return`
    Node *const expr;
    ReturnNode(const Span span, Node *const expr) : Node(span, NODE_RETURN), expr(expr) {}
};

struct ImportNode : public Node {
    // span is `import`
    Span *const parts;
    const i32 cnt;
    Span *const alias;
    ImportNode(const Span span, Span *const parts, const i32 cnt, Span *const alias)
        : Node(span, NODE_IMPORT), parts(parts), cnt(cnt), alias(alias)
    {
    }
};

struct ModuleNode : public Node {
    // span is filename
    Node *const *const decls;
    const i32 cnt;
    ModuleNode(const Span span, Node *const *const decls, const i32 cnt)
        : Node(span, NODE_MODULE), decls(decls), cnt(cnt)
    {
    }
};

struct AstVisitor {
    void visit_atom(AtomNode &node);
    void visit_list(ListNode &node);
    void visit_ident(IdentNode &node);
    void visit_unary(UnaryNode &node);
    void visit_binary(BinaryNode &node);
    void visit_selector(SelectorNode &node);
    void visit_subscr(SubscrNode &node);
    void visit_assign(AssignNode &node);
    void visit_call(CallNode &node);
    void visit_expr(Node &node);

    void visit_var_decl(VarDeclNode &node);
    void visit_fn_decl(FnDeclNode &node);
    void visit_class_decl(ClassDeclNode &node);
    void visit_expr_stmt(ExprStmtNode &node);
    void visit_block(BlockNode &node);
    void visit_if(IfNode &node);
    void visit_print(PrintNode &node);
    void visit_stmt(Node &Node);
};
