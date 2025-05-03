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
    [TOKEN_OR]         = {3, 4},
    [TOKEN_AND]        = {5, 6},
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
    [TOKEN_VAR]        = {0, 0},
    [TOKEN_IF]         = {0, 0},
    [TOKEN_ELSE]       = {0, 0},
    [TOKEN_SEMI]       = {0, 0},
    [TOKEN_COLON]      = {0, 0},
    [TOKEN_TY_NUM]     = {0, 0},
    [TOKEN_TY_BOOL]    = {0, 0},
    [TOKEN_EOF]        = {0, 0},
    [TOKEN_ERR]        = {0, 0},
};

static struct LiteralNode *mk_literal(struct Arena *arena, struct Token token) {
    struct LiteralNode *node = push_arena(arena, sizeof(struct LiteralNode));
    node->base.tag = NODE_LITERAL;
    node->base.span = token.span;
    node->lit_tag = token.tag;
    return node;
}

static struct IdentNode *mk_ident(struct Arena *arena, struct Token token) {
    struct IdentNode *node = push_arena(arena, sizeof(struct IdentNode));
    node->base.tag = NODE_IDENT;
    node->base.span = token.span;
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

static struct BinaryNode *mk_binary(struct Arena *arena, struct Token token, struct Node *lhs, struct Node *rhs) {
    struct BinaryNode *node = push_arena(arena, sizeof(struct BinaryNode));
    node->base.tag = NODE_BINARY;
    node->base.span = token.span;
    node->op_tag = token.tag;
    node->lhs = lhs;
    node->rhs = rhs;
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

static struct ExprStmtNode *mk_expr_stmt(struct Arena *arena, struct Span span, struct Node *expr) {
    struct ExprStmtNode *node = push_arena(arena, sizeof(struct ExprStmtNode));
    node->base.tag = NODE_EXPR_STMT;
    node->base.span = span;
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

// discard tokens until reach a starting (e.g. var, if) token or exit scope
static void recover(struct Parser *parser) {
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
    struct Node *lhs = NULL;
    switch (token.tag) {
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NUMBER:
        bump(parser);
        lhs = (struct Node*)mk_literal(&parser->arena, token);
        break;
    case TOKEN_IDENTIFIER:
        bump(parser);
        lhs = (struct Node*)mk_ident(&parser->arena, token);
        break;
    case TOKEN_L_PAREN:
        bump(parser);
        lhs = (struct Node*)parse_expr(parser, 0);
        expect(parser, TOKEN_R_PAREN, "expected `)`");
        break;
    case TOKEN_MINUS:
    case TOKEN_NOT:
        bump(parser);
        lhs = parse_expr(parser, 15);
        lhs = (struct Node*)mk_unary(&parser->arena, token, lhs);
        break;
    default:
        if (!parser->panic)
            push_errlist(&parser->errlist, token.span, "expected expression");
        parser->panic = true;
        return NULL;
    }
    while(true) {
        token = at(parser);
        u32 old_lvl = infix_prec[token.tag].old;
        u32 new_lvl = infix_prec[token.tag].new;
        if (old_lvl <= prec_lvl)
            break;
        bump(parser);
        struct Node *rhs = parse_expr(parser, new_lvl);
        lhs = (struct Node*) mk_binary(&parser->arena, token, lhs, rhs);
    }
    return lhs;
}

static struct TyNode *parse_ty_expr(struct Parser *parser) {
    struct Token token = at(parser);
    switch(token.tag) {
    case TOKEN_TY_NUM:
        bump(parser);
        return mk_primitive_ty(&parser->arena, TY_NUM);
    case TOKEN_TY_BOOL:
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
    struct Node *cond = parse_expr(parser, 0);
    expect(parser, TOKEN_R_PAREN, "expected `)`");
    struct BlockNode *thn = parse_block(parser);
    struct BlockNode *els = NULL;
    if (eat(parser, TOKEN_ELSE))
        els = parse_block(parser);
    return mk_if(&parser->arena, span, cond, thn, els);
}

// precondition: `var` token consumed
static struct VarDeclNode *parse_var_decl(struct Parser *parser){
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    struct TyNode *ty_hint = NULL;
    if (eat(parser, TOKEN_COLON))
        ty_hint = parse_ty_expr(parser);
    struct Node *init = NULL;
    if (eat(parser, TOKEN_EQ))
        init = parse_expr(parser, 0);
    expect(parser, TOKEN_SEMI, "expected `;`");
    return mk_var_decl(&parser->arena, span, ty_hint, init);
}

static struct BlockNode *parse_block(struct Parser *parser) {
    struct Span span = at(parser).span;
    if (!expect(parser, TOKEN_L_BRACE, "expected `{`"))
        return NULL;
    
    u32 count = 0;
    u32 cap = 8;
    struct Node **scratch = allocate(cap * sizeof(struct Node*));
    
    while(at(parser).tag != TOKEN_R_BRACE && at(parser).tag != TOKEN_EOF) {
        struct Node *node;
        if (at(parser).tag == TOKEN_L_BRACE) {
            node = (struct Node*)parse_block(parser);
        } else if (eat(parser, TOKEN_IF)) {
            node = (struct Node*)parse_if(parser);
        } else if (eat(parser, TOKEN_VAR)) {
            node = (struct Node*)parse_var_decl(parser);
        } else {
            node = parse_expr(parser, 0);
            if (node) {
                expect(parser, TOKEN_SEMI, "expected `;`"); 
                node = (struct Node*) mk_expr_stmt(&parser->arena, node->span, node);
            }   
        }
        if (count == cap) {
            cap *= 2;
            scratch = reallocate(scratch, cap * sizeof(struct Node*));
        }
        scratch[count] = node;
        count++;
        if (parser->panic)
            recover(parser);
    }
    expect(parser, TOKEN_R_BRACE, "expected `}`");

    struct Node **stmts = push_arena(&parser->arena, count * sizeof(struct Node*));
    for (i32 i = 0; i < count; i++)
        stmts[i] = scratch[i];
    release(scratch);

    return mk_block(&parser->arena, span, stmts, count);
}

void parse(struct Parser *parser) {
    struct Node *node = (struct Node*)parse_block(parser);
    parser->ast = node;
}