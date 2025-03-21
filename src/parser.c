#include <stdlib.h>
#include "../util/arena.h"
#include "error.h"
#include "scanner.h"
#include "parser.h"

struct PrecedenceLevel {
    u32 old;
    u32 new;
};

struct PrecedenceLevel infix_precedence[] = {
    [TOKEN_PLUS]        = {1, 2},
    [TOKEN_MINUS]       = {1, 2},
    [TOKEN_STAR]        = {3, 4},
    [TOKEN_SLASH]       = {3, 4},
    [TOKEN_LEFT_PAREN]  = {0, 0},
    [TOKEN_RIGHT_PAREN] = {0, 0},
    [TOKEN_NUMBER]      = {0, 0},
    [TOKEN_EOF]         = {0, 0},
    [TOKEN_ERROR]       = {0, 0},
};

static struct Token peek_next(struct Parser *parser) {
    return parser->next;
}

static struct Token advance(struct Parser *parser) {
    parser->current = parser->next;
    parser->next = next_token(&parser->scanner);
    return parser->current;
};

static struct LiteralExpr *make_literal_expr(struct Arena *arena, struct Token value) {
    struct LiteralExpr *expr = arena_push(arena, sizeof(struct LiteralExpr));
    expr->base.type = EXPR_LITERAL;
    expr->value = value;
    return expr;
}

static struct UnaryExpr *make_unary_expr(struct Arena *arena, struct Node *rhs, struct Token op) {
    struct UnaryExpr *expr = arena_push(arena, sizeof(struct UnaryExpr));
    expr->base.type = EXPR_UNARY;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}
 
static struct BinaryExpr *make_binary_expr(struct Arena *arena, struct Node *lhs, struct Node *rhs, struct Token op) {
    struct BinaryExpr *expr = arena_push(arena, sizeof(struct BinaryExpr));
    expr->base.type = EXPR_BINARY;
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

static struct Node *expression(struct Arena *arena, struct Parser *parser, u32 level) {
    struct Node *lhs = NULL;
    struct Token token = advance(parser);
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_IDENTIFIER:
            lhs = (struct Node*) make_literal_expr(arena, token);
            break;
        case TOKEN_LEFT_PAREN:
            lhs = expression(arena, parser, 0);
            if (advance(parser).type != TOKEN_RIGHT_PAREN) {
                error(parser, "unmatched ')'");
            }
            break;
        case TOKEN_MINUS:
            lhs = (struct Node*) make_unary_expr(arena, expression(arena, parser, 5), token);
            break;
        default:
            error(parser, "invalid syntax");
            return NULL;
    }

    while(true) {
        token = peek_next(parser);
        u32 old_level = infix_precedence[token.type].old;
        u32 new_level = infix_precedence[token.type].new;
        if (old_level <= level) {
            break;
        }
        advance(parser);
        struct Node *rhs = expression(arena, parser, new_level);
        lhs = (struct Node*) make_binary_expr(arena, lhs, rhs, token);
    }
    return lhs;
}

struct Node *parse(struct Arena *arena, const char *source) {
    struct Scanner scanner;
    init_scanner(&scanner, source);

    struct Parser parser;
    parser.scanner = scanner;
    parser.next = next_token(&parser.scanner);
    parser.had_error = false;
    parser.panic = false;

    struct Node *expr = expression(arena, &parser, 0);
    if (advance(&parser).type != TOKEN_EOF) {
        error(&parser, "invalid syntax");
    }
    return parser.had_error ? NULL : expr;
}