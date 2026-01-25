#pragma once
#include "scan.h"
#define MAX_LOCALS    (256)
#define FLAG_NONE     (0)
#define FLAG_CAPTURED (1 << 1)
#define FLAG_METHOD   (1 << 2)
#define FLAG_INIT     (1 << 3)

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
    NODE_CAPTURE_DECL,
    NODE_EXPR_STMT,
    NODE_RETURN,
    NODE_BLOCK,
    NODE_IF,
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

enum LocTag { LOC_LOCAL, LOC_GLOBAL, LOC_STACK_HEAPVAL, LOC_CAPTURED_HEAPVAL };

struct Loc {
    LocTag tag;
    i32 idx;
};

struct DeclNode;

struct IdentNode : public Node {
    // span is identifier
    DeclNode *decl; // VarDeclNode, FnDeclNode, or ClassDeclNode
    IdentNode(const Span span) : Node(span, NODE_IDENT), decl(nullptr) {}
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

struct DeclNode : public Node {
    i32 fn_depth;
    u32 flags;
    Loc loc;
    DeclNode(const Span span, const NodeTag tag) : Node(span, tag), fn_depth(0), flags(FLAG_NONE){};
};

struct VarDeclNode : public DeclNode {
    // span is identifier
    Node *const init;
    VarDeclNode(const Span span, Node *const init) : DeclNode(span, NODE_VAR_DECL), init(init) {}
};

struct CaptureDecl : public DeclNode {
    DeclNode *const decl_original; // declaration of the variable that is captured
    CaptureDecl(const Span span, DeclNode *const decl_original)
        : DeclNode(span, NODE_CAPTURE_DECL), decl_original(decl_original){};
};

struct FnDeclNode : public DeclNode {
    // span is identifier
    BlockNode *const body;
    VarDeclNode *const params;
    const i32 arity;
    i32 capture_cnt;
    CaptureDecl *captures[MAX_LOCALS];
    FnDeclNode(const Span span, BlockNode *const body, VarDeclNode *const params, const i32 arity)
        : DeclNode(span, NODE_FN_DECL), body(body), params(params), arity(arity), capture_cnt(0), captures{}
    {
    }
};

struct ClassDeclNode : public DeclNode {
    // span is identifier
    FnDeclNode *const *const methods;
    const i32 cnt;
    ClassDeclNode(const Span span, FnDeclNode *const *const methods, const i32 cnt)
        : DeclNode(span, NODE_CLASS_DECL), methods(methods), cnt(cnt)
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
    const bool is_fn_body;
    i32 local_cnt; // locals declared in block, including params if block is fn body
    BlockNode(const Span span, Node *const *const stmts, const i32 cnt, const bool is_fn_body)
        : Node(span, NODE_BLOCK), stmts(stmts), cnt(cnt), is_fn_body(is_fn_body), local_cnt(0)
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

struct ImportNode : public DeclNode {
    // span is `import`
    Span *const parts;
    const i32 cnt;
    Span *const alias;
    ImportNode(const Span span, Span *const parts, const i32 cnt, Span *const alias)
        : DeclNode(span, NODE_IMPORT), parts(parts), cnt(cnt), alias(alias)
    {
    }
};

struct ModuleNode : public Node {
    // span is filename
    DeclNode *const *const decls;
    const i32 cnt;
    ModuleNode(const Span span, DeclNode *const *const decls, const i32 cnt)
        : Node(span, NODE_MODULE), decls(decls), cnt(cnt)
    {
    }
};

struct AstVisitor {
    virtual void visit_atom(AtomNode &node);
    virtual void visit_list(ListNode &node);
    virtual void visit_ident(IdentNode &node);
    virtual void visit_unary(UnaryNode &node);
    virtual void visit_binary(BinaryNode &node);
    virtual void visit_selector(SelectorNode &node);
    virtual void visit_subscr(SubscrNode &node);
    virtual void visit_assign(AssignNode &node);
    virtual void visit_call(CallNode &node);
    virtual void visit_expr(Node &node);

    virtual void visit_var_decl(VarDeclNode &node);
    virtual void visit_fn_decl(FnDeclNode &node);
    virtual void visit_class_decl(ClassDeclNode &node);
    virtual void visit_expr_stmt(ExprStmtNode &node);
    virtual void visit_return(ReturnNode &node);
    virtual void visit_block(BlockNode &node);
    virtual void visit_if(IfNode &node);
    virtual void visit_print(PrintNode &node);
    virtual void visit_stmt(Node &node);

    virtual ~AstVisitor(){};
};
