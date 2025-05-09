#include <stdlib.h>
#include "memory.h"
#include "parse.h"

struct PrecLvl {
    u32 old;
    u32 new;
};

struct PrecLvl infix_prec[] = {
    [TOKEN_EQ]         = {1, 2},
    [TOKEN_PLUS_EQ]    = {1, 2},
    [TOKEN_MINUS_EQ]   = {1, 2},
    [TOKEN_STAR_EQ]    = {1, 2},
    [TOKEN_SLASH_EQ]   = {1, 2},
    [TOKEN_OR]         = {3, 3},
    [TOKEN_AND]        = {5, 5},
    [TOKEN_EQ_EQ]      = {7, 8},
    [TOKEN_NEQ]        = {7, 8},
    [TOKEN_LT]         = {9, 10},
    [TOKEN_LEQ]        = {9, 10},
    [TOKEN_GT]         = {9, 10},
    [TOKEN_GEQ]        = {9, 10},
    [TOKEN_PLUS]       = {11, 12},
    [TOKEN_MINUS]      = {11, 12},
    [TOKEN_STAR]       = {13, 14},
    [TOKEN_SLASH]      = {13, 14},
    [TOKEN_NOT]        = {0, 0},
    [TOKEN_L_PAREN]    = {0, 0},
    [TOKEN_L_PAREN]    = {0, 0},
    [TOKEN_R_PAREN]    = {0, 0},
    [TOKEN_L_BRACE]    = {0, 0},
    [TOKEN_R_BRACE]    = {0, 0},
    [TOKEN_NUMBER]     = {0, 0},
    [TOKEN_TRUE]       = {0, 0},
    [TOKEN_FALSE]      = {0, 0},
    [TOKEN_IDENTIFIER] = {0, 0},
    [TOKEN_FN]         = {0, 0},
    [TOKEN_RETURN]     = {0, 0},
    [TOKEN_VAR]        = {0, 0},
    [TOKEN_IF]         = {0, 0},
    [TOKEN_ELSE]       = {0, 0},
    [TOKEN_SEMI]       = {0, 0},
    [TOKEN_COLON]      = {0, 0},
    [TOKEN_NUM]        = {0, 0},
    [TOKEN_BOOL]       = {0, 0},
    [TOKEN_EOF]        = {0, 0},
    [TOKEN_ERR]        = {0, 0},
};

// dynarray of Node*
struct NodeArray {
    u32 cap;
    u32 count;
    struct Node **nodes;
};

static void init_node_array(struct NodeArray *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->nodes = allocate(arr->cap * sizeof(struct Node*));
}

static void release_node_array(struct NodeArray *arr) {
    arr->count = 0;
    arr->cap = 0;
    release(arr->nodes);
    arr->nodes = NULL;
}

static void push_node_array(struct NodeArray *arr, struct Node *node) {
    if (arr->count == arr->cap) {
        arr->cap *= 2;
        arr->nodes = reallocate(arr->nodes, arr->cap * sizeof(struct Node*));
    }
    arr->nodes[arr->count] = node;
    arr->count++;
}

static struct LiteralNode *mk_literal(struct Arena *arena, struct Token token) {
    struct LiteralNode *node = push_arena(arena, sizeof(struct LiteralNode));
    node->base.tag = NODE_LITERAL;
    node->base.span = token.span;
    node->lit_tag = token.tag;
    return node;
}

static struct IdentNode *mk_ident(struct Arena *arena, struct Span span) {
    struct IdentNode *node = push_arena(arena, sizeof(struct IdentNode));
    node->base.tag = NODE_IDENT;
    node->base.span = span;
    node->id = -1;
    return node;
}

static struct UnaryNode *mk_unary(struct Arena *arena, struct Token token, struct Node *rhs) {
    struct UnaryNode *node = push_arena(arena, sizeof(struct UnaryNode));
    node->base.tag = NODE_UNARY;
    node->base.span = token.span;
    node->op_tag = token.tag;
    node->rhs = rhs;
    return node;
}

static struct BinaryNode *mk_binary(struct Arena *arena, struct Span span, enum TokenTag tag, struct Node *lhs, struct Node *rhs) {
    struct BinaryNode *node = push_arena(arena, sizeof(struct BinaryNode));
    node->base.tag = NODE_BINARY;
    node->base.span = span;
    node->op_tag = tag;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static struct FnCallNode *mk_fn_call(struct Arena *arena, struct Span span, struct Node *lhs, struct Node **args, u32 arity) {
    struct FnCallNode *node = push_arena(arena, sizeof(struct FnCallNode));
    node->base.tag = NODE_FN_CALL;
    node->base.span = span;
    node->lhs = lhs;
    node->args = args;
    node->arity = arity;
    return node;
}

static struct VarDeclNode *mk_var_decl(struct Arena *arena, struct Span span, struct TyNode *ty, struct Node *init) {
    struct VarDeclNode *node = push_arena(arena, sizeof(struct VarDeclNode));
    node->base.tag = NODE_VAR_DECL;
    node->base.span = span;
    node->id = -1;
    node->ty_hint = ty;
    node->init = init;
    return node;
}

static struct FnDeclNode *mk_fn_decl(struct Arena *arena, struct Span span, struct IdentNode **param_names, struct TyNode **param_tys, u32 arity, struct BlockNode *body, struct TyNode *ret_ty) {
    struct FnDeclNode *node = push_arena(arena, sizeof(struct FnDeclNode));
    node->base.tag = NODE_FN_DECL;
    node->base.span = span;
    node->param_names = param_names;
    node->param_tys = param_tys;
    node->arity = arity;
    node->body = body;
    node->ret_ty = ret_ty;
    return node;
}

static struct ExprStmtNode *mk_expr_stmt(struct Arena *arena, struct Node *expr) {
    struct ExprStmtNode *node = push_arena(arena, sizeof(struct ExprStmtNode));
    node->base.tag = NODE_EXPR_STMT;
    node->base.span.length = 0;
    node->base.span.start = NULL;
    node->expr = expr;
    return node;
}

static struct IfNode *mk_if(struct Arena *arena, struct Span span, struct Node *cond, struct BlockNode *thn, struct BlockNode *els) {
    struct IfNode *node = push_arena(arena, sizeof(struct IfNode));
    node->base.tag = NODE_IF;
    node->base.span = span;
    node->cond = cond;
    node->thn = thn;
    node->els = els;
    return node;
}

static struct BlockNode *mk_block(struct Arena *arena, struct Span span, struct Node **stmts, u32 count) {
    struct BlockNode *node = push_arena(arena, sizeof(struct BlockNode));
    node->base.tag = NODE_BLOCK;
    node->base.span = span;
    node->stmts = stmts;
    node->count = count;
    return node;
}

static struct FileNode *mk_file(struct Arena *arena, struct Node **stmts, u32 count) {
    struct FileNode *node = push_arena(arena, sizeof(struct FileNode));
    node->base.tag = NODE_FILE;
    node->base.span.length = 0;
    node->base.span.start = NULL;
    node->stmts = stmts;
    node->count = count;
    return node;
}

void init_parser(struct Parser *parser, const char *source) {
    init_errlist(&parser->errlist);
    init_arena(&parser->arena);
    init_scanner(&parser->scanner, source);
    parser->at = next_token(&parser->scanner);
    parser->panic = false;
}

void release_parser(struct Parser *parser) {
    release_errlist(&parser->errlist);
    release_arena(&parser->arena);
}

static struct Token at(struct Parser *parser) {
    return parser->at;
}

static struct Token prev(struct Parser *parser) {
    return parser->prev;
}

static void bump(struct Parser *parser) {
    parser->prev = parser->at;
    parser->at = next_token(&parser->scanner);
}

static bool eat(struct Parser *parser, enum TokenTag tag) {
    if (at(parser).tag == tag) {
        bump(parser);
        return true;
    }
    return false;
}

static bool expect(struct Parser *parser, enum TokenTag tag, const char *msg) {
    if (eat(parser, tag))
        return true;
    if (!parser->panic)
        push_errlist(&parser->errlist, at(parser).span, msg);
    parser->panic = true;
    return false;
}

static void advance_with_err(struct Parser *parser, const char *msg) {
    bump(parser);
    if (!parser->panic)
        push_errlist(&parser->errlist, at(parser).span, msg);
    parser->panic = true;
}

// discard tokens until reach a starting (e.g. var, if) token or exit scope
static void recover_block(struct Parser *parser) {
    parser->panic = false;
    i32 depth = 0;
    while (prev(parser).tag != TOKEN_SEMI && at(parser).tag != TOKEN_EOF) {
        switch(at(parser).tag) {
        case TOKEN_IF:
        case TOKEN_VAR:
            return;
        case TOKEN_L_BRACE:
            depth++;
            break;
        case TOKEN_R_BRACE:
            depth--;
        default:
            break;
        }
        if (depth == -1)
            return;
        bump(parser);
    }
}

static struct BlockNode *parse_block(struct Parser *parser);

static struct Node *parse_expr(struct Parser *parser, u32 prec_lvl) {
    struct Token token = at(parser);
    bump(parser);
    struct Node *lhs = NULL;
    switch (token.tag) {
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NUMBER:
        lhs = (struct Node*)mk_literal(&parser->arena, token);
        break;
    case TOKEN_IDENTIFIER:
        lhs = (struct Node*)mk_ident(&parser->arena, token.span);
        break;
    case TOKEN_L_PAREN:
        lhs = (struct Node*)parse_expr(parser, 1);
        expect(parser, TOKEN_R_PAREN, "expected `)`");
        break;
    case TOKEN_MINUS:
    case TOKEN_NOT:
        lhs = parse_expr(parser, 15);
        lhs = (struct Node*)mk_unary(&parser->arena, token, lhs);
        break;
    default:
        if (!parser->panic)
            push_errlist(&parser->errlist, token.span, "expected expression");
        parser->panic = true;
        return NULL;
    }
    // parse fn calls
    while(eat(parser, TOKEN_L_PAREN)) {
        struct Span fn_call_span = prev(parser).span;
        struct NodeArray scratch_args;
        init_node_array(&scratch_args);
        while(at(parser).tag != TOKEN_R_PAREN && at(parser).tag != TOKEN_EOF) {
            struct Node *arg = parse_expr(parser, 1);
            push_node_array(&scratch_args, arg);
            if (at(parser).tag != TOKEN_R_PAREN)
                expect(parser, TOKEN_COMMA, "expected `,`");
        }
        expect(parser, TOKEN_R_PAREN, "expected `)`");
        struct Node** args = push_arena(&parser->arena, scratch_args.count * sizeof(struct Node*));
        for (i32 i = 0; i < scratch_args.count; i++)
            args[i] = scratch_args.nodes[i];
        lhs = (struct Node*)mk_fn_call(&parser->arena, fn_call_span, lhs, args, scratch_args.count);
        release_node_array(&scratch_args);
    }
    while(true) {
        token = at(parser);
        u32 old_lvl = infix_prec[token.tag].old;
        u32 new_lvl = infix_prec[token.tag].new;
        if (old_lvl < prec_lvl) {
            break;
        }
        bump(parser);
        struct Node *rhs = parse_expr(parser, new_lvl);
        
        enum TokenTag tag = token.tag;
        // desugar +=, -=, *=, /=
        switch(tag) {
        case TOKEN_PLUS_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_PLUS, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        case TOKEN_MINUS_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_MINUS, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        case TOKEN_STAR_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_STAR, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        case TOKEN_SLASH_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_SLASH, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        }
        lhs = (struct Node*) mk_binary(&parser->arena, token.span, tag, lhs, rhs);
    }
    return lhs;
}

static struct TyNode *parse_ty_expr(struct Parser *parser) {
    struct Token token = at(parser);
    switch(token.tag) {
    case TOKEN_NUM:
        bump(parser);
        return mk_primitive_ty(&parser->arena, TY_NUM);
    case TOKEN_BOOL:
        bump(parser);
        return mk_primitive_ty(&parser->arena, TY_BOOL);
    default:
        if (!parser->panic)
            push_errlist(&parser->errlist, token.span, "expected type");
        parser->panic = true;
        return NULL;
    }
}

// precondition: `if` token consumed
static struct IfNode *parse_if(struct Parser *parser) {
    struct Span span = prev(parser).span;
    expect(parser, TOKEN_L_PAREN, "expected `(`");
    struct Node *cond = parse_expr(parser, 1);
    expect(parser, TOKEN_R_PAREN, "expected `)`");
    struct BlockNode *thn = parse_block(parser);
    struct BlockNode *els = NULL;
    if (eat(parser, TOKEN_ELSE))
        els = parse_block(parser);
    return mk_if(&parser->arena, span, cond, thn, els);
}

// precondition: `var` token consumed
static struct VarDeclNode *parse_var_decl(struct Parser *parser) {
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    struct TyNode *ty_hint = NULL;
    if (eat(parser, TOKEN_COLON))
        ty_hint = parse_ty_expr(parser);
    struct Node *init = NULL;
    if (eat(parser, TOKEN_EQ))
        init = parse_expr(parser, 1);
    expect(parser, TOKEN_SEMI, "expected `;`");
    return mk_var_decl(&parser->arena, span, ty_hint, init);
}

// precondition: `fn` token consumed
static struct FnDeclNode *parse_fn_decl(struct Parser *parser) {
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    expect(parser, TOKEN_L_PAREN, "expected `(`");  
    struct NodeArray scratch_names;
    struct NodeArray scratch_tys;
    init_node_array(&scratch_names);
    init_node_array(&scratch_tys);
    while(at(parser).tag != TOKEN_R_PAREN && at(parser).tag != TOKEN_EOF) {
        if (eat(parser, TOKEN_IDENTIFIER)) {
            struct IdentNode *ident = mk_ident(&parser->arena, prev(parser).span);
            expect(parser, TOKEN_COLON, "expected `:`");
            struct TyNode *ty = parse_ty_expr(parser);
            if (at(parser).tag != TOKEN_R_PAREN)
                expect(parser, TOKEN_COMMA, "expected `,`");
            push_node_array(&scratch_names, (struct Node*)ident);
            push_node_array(&scratch_tys, (struct Node*)ty);
        } else {
            advance_with_err(parser, "expected identifier");
        }
    }
    expect(parser, TOKEN_R_PAREN, "expected `)`");
    u32 arity = scratch_names.count;
    struct IdentNode **param_names = push_arena(&parser->arena, arity * sizeof(struct IdentNode*));
    struct TyNode **param_tys = push_arena(&parser->arena, arity * sizeof(struct TyNode*));
    for (i32 i = 0; i < arity; i++) {
        param_names[i] = (struct IdentNode*)scratch_names.nodes[i];
        param_tys[i] = (struct TyNode*)scratch_tys.nodes[i];
    }
    release_node_array(&scratch_names);
    release_node_array(&scratch_tys);
    struct TyNode *ret_ty;
    if (at(parser).tag == TOKEN_L_BRACE)
        ret_ty = mk_primitive_ty(&parser->arena, TY_VOID);
    else 
        ret_ty = parse_ty_expr(parser);
    struct BlockNode *body = parse_block(parser);
    return mk_fn_decl(&parser->arena, span, param_names, param_tys, arity, body, ret_ty);
}

// precondition: `return` token consumed
static struct ReturnNode *parse_return(struct Parser *parser) {
    struct Span span = prev(parser).span;
    struct Node *expr = parse_expr(parser, 1);
    struct ReturnNode *node = push_arena(&parser->arena, sizeof(struct ReturnNode));
    node->base.tag = NODE_RETURN;
    node->base.span = span;
    node->expr = expr;
    expect(parser, TOKEN_SEMI, "expected `;`");
    return node;
}

// TEMP remove when we add functions
// precondition: `print` token consumed
static struct PrintNode *parse_print(struct Parser *parser) {
    struct Span span = prev(parser).span;
    struct Node *expr = parse_expr(parser, 1);
    struct PrintNode *node = push_arena(&parser->arena, sizeof(struct PrintNode));
    node->base.tag = NODE_PRINT;
    node->base.span = span;
    node->expr = expr;
    expect(parser, TOKEN_SEMI, "expected `;`");
    return node;
}

static struct BlockNode *parse_block(struct Parser *parser) {
    struct Span span = at(parser).span;
    if (!expect(parser, TOKEN_L_BRACE, "expected `{`"))
        return NULL;
    struct NodeArray scratch;
    init_node_array(&scratch);
    while(at(parser).tag != TOKEN_R_BRACE && at(parser).tag != TOKEN_EOF) {
        struct Node *node;
        if (at(parser).tag == TOKEN_L_BRACE) {
            node = (struct Node*)parse_block(parser);
        } else if (eat(parser, TOKEN_IF)) {
            node = (struct Node*)parse_if(parser);
        } else if (eat(parser, TOKEN_VAR)) {
            node = (struct Node*)parse_var_decl(parser);
        } else if (eat(parser, TOKEN_RETURN)) {
            node = (struct Node*)parse_return(parser);  
        } else if (eat(parser, TOKEN_PRINT)) {
            node = (struct Node*)parse_print(parser);
        } else {
            node = parse_expr(parser, 1);
            expect(parser, TOKEN_SEMI, "expected `;`"); 
            node = (struct Node*) mk_expr_stmt(&parser->arena, node); 
        }
        push_node_array(&scratch, node);
        if (parser->panic)
            recover_block(parser);
    }
    expect(parser, TOKEN_R_BRACE, "expected `}`");
    u32 count = scratch.count;
    struct Node **stmts = push_arena(&parser->arena, count * sizeof(struct Node*));
    for (i32 i = 0; i < count; i++)
        stmts[i] = scratch.nodes[i];
    release_node_array(&scratch);
    return mk_block(&parser->arena, span, stmts, count);
}

static struct FileNode *parse_file(struct Parser *parser) {
    struct NodeArray scratch;
    init_node_array(&scratch);
    while (at(parser).tag != TOKEN_EOF) {
        expect(parser, TOKEN_FN, "expected function declaration");
        struct Node *node = (struct Node*)parse_fn_decl(parser);
        push_node_array(&scratch, node);
    }
    u32 count = scratch.count;
    struct Node **stmts = push_arena(&parser->arena, count * sizeof(struct Node*));
    for (i32 i = 0; i < count; i++)
        stmts[i] = scratch.nodes[i];
    release_node_array(&scratch);
    return mk_file(&parser->arena, stmts, count);
}

// TODO disallow trailing comma in function declarations and calls
void parse(struct Parser *parser) {
    parser->ast = (struct Node*)parse_file(parser);
}