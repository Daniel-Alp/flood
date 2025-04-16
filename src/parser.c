#include <stdlib.h>
#include "../util/arena.h"
#include "parser.h"
#include "error.h"

struct PrecLvl {
    u32 old;
    u32 new;
};

struct PrecLvl infix_prec[] = {
    [TOKEN_PLUS]            = {11, 12}, 
    [TOKEN_MINUS]           = {11, 12}, 
    [TOKEN_STAR]            = {13, 14}, 
    [TOKEN_SLASH]           = {13, 14}, 
    [TOKEN_EQUAL]           = {1, 2},
    [TOKEN_LESS_EQUAL]      = {9, 10}, 
    [TOKEN_GREATER_EQUAL]   = {9, 10}, 
    [TOKEN_LESS]            = {9, 10}, 
    [TOKEN_GREATER]         = {9, 10}, 
    [TOKEN_EQUAL_EQUAL]     = {7, 8},  
    [TOKEN_NOT_EQUAL]       = {7, 8},
    [TOKEN_AND]             = {5, 6}, 
    [TOKEN_OR]              = {3, 4}, 
    [TOKEN_NOT]             = {0, 0}, 
    [TOKEN_NUMBER]          = {0, 0}, 
    [TOKEN_IDENTIFIER]      = {0, 0}, 
    [TOKEN_TRUE]            = {0, 0}, 
    [TOKEN_FALSE]           = {0, 0}, 
    [TOKEN_LEFT_PAREN]      = {0, 0}, 
    [TOKEN_RIGHT_PAREN]     = {0, 0}, 
    [TOKEN_LEFT_BRACE]      = {0, 0}, 
    [TOKEN_RIGHT_BRACE]     = {0, 0},
    [TOKEN_SEMICOLON]       = {0, 0},
    [TOKEN_IF]              = {0, 0}, 
    [TOKEN_ELSE]            = {0, 0},
    [TOKEN_VAR]             = {0, 0},
    [TOKEN_EOF]             = {0, 0},
    [TOKEN_ERROR]           = {0, 0}
};

static struct LiteralExpr *mk_literal(struct Arena *arena, struct Token val) {
    struct LiteralExpr *expr = arena_push(arena, sizeof(struct LiteralExpr));
    expr->base.type = EXPR_LITERAL;
    expr->val = val;
    return expr;
}

struct UnaryExpr *mk_unary(struct Arena *arena, struct Node *rhs, struct Token op) {
    struct UnaryExpr *expr = arena_push(arena, sizeof(struct UnaryExpr));
    expr->base.type = EXPR_UNARY;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

struct BinaryExpr *mk_binary(struct Arena *arena, struct Node *lhs, struct Node *rhs, struct Token op) {
    struct BinaryExpr *expr = arena_push(arena, sizeof(struct BinaryExpr));
    expr->base.type = EXPR_BINARY;
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

static struct IfExpr *mk_if(struct Arena *arena, struct Node *cond, struct BlockExpr *thn, struct BlockExpr *els) {
    struct IfExpr *expr = arena_push(arena, sizeof(struct IfExpr));
    expr->base.type = EXPR_IF;
    expr->cond = cond;
    expr->thn = thn;
    expr->els = els;
    return expr;
}

static struct VarStmt *mk_var(struct Arena *arena, struct Token id, struct Node *init) {
    struct VarStmt *stmt = arena_push(arena, sizeof(struct VarStmt));
    stmt->base.type = STMT_VAR;
    stmt->id = id;
    stmt->init = init;
    return stmt;
}

static struct BlockExpr *mk_block(struct Arena *arena, struct NodeList *stmts) {
    struct BlockExpr *stmt = arena_push(arena, sizeof(struct BlockExpr));
    stmt->base.type = EXPR_BLOCK;
    stmt->stmts = stmts;
    return stmt;
}

static struct ExprStmt *mk_expr_stmt(struct Arena *arena, struct Node *expr) {
    struct ExprStmt *stmt = arena_push(arena, sizeof(struct ExprStmt));
    stmt->base.type = STMT_EXPR;
    stmt->expr = expr;
    return stmt;
}

static struct Token at(struct Parser *parser) {
    return parser->at;
}

static void bump(struct Parser *parser) {
    parser->at = next_token(&(parser->scanner));
}

// advance if current token type equals expected type
static bool eat(struct Parser *parser, enum TokenType type) {
    if (at(parser).type == type) {
        bump(parser);
        return true;
    }
    return false;
}

static bool expect(struct Parser *parser, enum TokenType type, const char *msg) {
    if (eat(parser, type))
        return true;
    emit_parse_error(parser->at, msg, parser);
    return false;
}

// discard unmatched tokens until at unmatched `}` or EOF
static void recover(struct Parser *parser) {
    i32 depth = 0;
    enum TokenType type;
    while (true) {
        type = at(parser).type;
        if (type == TOKEN_LEFT_BRACE)
            depth++;
        if (type == TOKEN_RIGHT_BRACE)
            depth--;
        if (depth == -1 || type == TOKEN_EOF)
            break;
        bump(parser);
    }
}

static struct IfExpr *parse_if(struct Arena *arena, struct Parser *parser);

static struct BlockExpr *parse_block(struct Arena *arena, struct Parser *parser);

static struct VarStmt *parse_var(struct Arena *arena, struct Parser *parser);

static struct Node *parse_expr(struct Arena *arena, struct Parser *parser, u32 prec_lvl) {
    struct Node *lhs = NULL;
    struct Token token = at(parser);
    switch (token.type) {
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NUMBER:
        case TOKEN_IDENTIFIER:
            bump(parser);
            lhs = (struct Node*) mk_literal(arena, token);
            break;
        case TOKEN_LEFT_PAREN:
            bump(parser);
            lhs = parse_expr(arena, parser, 0);
            if (!lhs || !expect(parser, TOKEN_RIGHT_PAREN, "expected `)`"))
                return NULL;
            break;
        case TOKEN_MINUS:
        case TOKEN_NOT:
            bump(parser);
            lhs = parse_expr(arena, parser, 15);
            if (!lhs)
                return NULL;
            lhs = (struct Node*) mk_unary(arena, lhs, token);
            break;
        case TOKEN_IF:
            lhs = (struct Node*) parse_if(arena, parser);
            if (!lhs)
                return NULL;
            break;
        case TOKEN_LEFT_BRACE:
            lhs = (struct Node*) parse_block(arena, parser);
            break;
        default:
            emit_parse_error(token, "unexpected token", parser);
            return NULL;
    }
    while(true) {
        token = at(parser);
        u32 old_lvl = infix_prec[token.type].old;
        u32 new_lvl = infix_prec[token.type].new;
        if (old_lvl <= prec_lvl)
            break;
        bump(parser);
        struct Node *rhs = parse_expr(arena, parser, new_lvl);
        if (!rhs)
            return NULL;
        lhs = (struct Node*) mk_binary(arena, lhs, rhs, token);
    }
    return lhs;
}

static struct BlockExpr *parse_block(struct Arena *arena, struct Parser *parser) {
    if (!expect(parser, TOKEN_LEFT_BRACE, "expected `{`"))
        return NULL;
    struct NodeList *stmts = arena_push(arena, sizeof(struct NodeList));
    struct BlockExpr *block = mk_block(arena, stmts);
    while (!eat(parser, TOKEN_RIGHT_BRACE)) {
        enum TokenType type = at(parser).type;
        struct Node *node;
        if (type == TOKEN_EOF) {
            emit_parse_error(at(parser), "unexpected EOF", parser);
            break;
        } else if (type == TOKEN_SEMICOLON) {
            bump(parser);
            continue;
        } else if (type == TOKEN_LEFT_BRACE) {
            node = (struct Node*) parse_block(arena, parser);
        } else if (type == TOKEN_IF) {
            node = (struct Node*) parse_if(arena, parser);
        } else if (type == TOKEN_VAR) {
            node = (struct Node*) parse_var(arena, parser);
        } else {
            node = parse_expr(arena, parser, 0);
            if (node && at(parser).type != TOKEN_RIGHT_BRACE && expect(parser, TOKEN_SEMICOLON, "expected `;`"))
                node = (struct Node*) mk_expr_stmt(arena, node);
        }
        if (!node)
            recover(parser);
        stmts->node = node;
        stmts->next = arena_push(arena, sizeof(struct NodeList));
        stmts = stmts->next;
    }
    return block;
}

static struct IfExpr *parse_if(struct Arena *arena, struct Parser *parser) {
    if (!expect(parser, TOKEN_IF, "expected `if`"))
        return NULL;
    struct Node *cond = parse_expr(arena, parser, 0);
    if (!cond)
        return NULL;
    struct BlockExpr *thn = parse_block(arena, parser);
    if (!thn)
        return NULL;
    struct BlockExpr *els = NULL;
    if (eat(parser, TOKEN_ELSE)) {
        els = parse_block(arena, parser);
        if (!els)
            return NULL;
    }
    return mk_if(arena, cond, thn, els);
}

static struct VarStmt *parse_var(struct Arena *arena, struct Parser *parser) {
    if (!expect(parser, TOKEN_VAR, "expected `var`"))
        return NULL;
    struct Token id = at(parser);
    if (id.type != TOKEN_IDENTIFIER) {
        emit_parse_error(id, "expected identifier", parser);
        return NULL;
    }
    bump(parser);
    struct Node *init = NULL;
    if (eat(parser, TOKEN_EQUAL)) {
        init = parse_expr(arena, parser, 0);
        if (!init)
            return NULL;
    }
    if (!expect(parser, TOKEN_SEMICOLON, "expected `;`"))  
        return NULL;
    return mk_var(arena, id, init);
}

struct BlockExpr *parse(struct Arena *arena, const char *source) {
    struct Scanner scanner;
    init_scanner(&scanner, source);
    struct Token at = next_token(&scanner);
    struct Parser parser = {
        .scanner = scanner,
        .at = at, 
        .had_error = false
    };
    struct BlockExpr *block = parse_block(arena, &parser);
    return parser.had_error ? NULL : block;
}