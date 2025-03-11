#include <stdlib.h>
#include "../util/arena.h"
#include "scanner.h"
#include "parser.h"

struct PrecedenceLevel {
    u32 current;
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

static struct LiteralExpr *make_literal_expr(struct Arena *arena, struct Token value) {
    struct LiteralExpr *expr = arena_push(arena, sizeof(struct LiteralExpr));
    expr->base.type = EXPR_LITERAL;
    expr->value = value;
    return expr;
}
 
static struct BinaryExpr *make_binary_expr(struct Arena *arena, struct Expr *lhs, struct Expr *rhs, struct Token op) {
    struct BinaryExpr *expr = arena_push(arena, sizeof(struct BinaryExpr));
    expr->base.type = EXPR_BINARY;
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

static struct Expr *expression(struct Arena *arena, struct Scanner *scanner, u32 level) {
    struct Expr *lhs = NULL;
    struct Token token = next_token(scanner);
    switch (token.type) {
        case TOKEN_NUMBER:
            lhs = (struct Expr*) make_literal_expr(arena, token);
            break;
        case TOKEN_LEFT_PAREN:
            lhs = expression(arena, scanner, 0);
            next_token(scanner);
            break;
        default:
            exit(1);
    }

    while(true) {
        const char *prev_current = scanner->current;
        token = next_token(scanner);
        
        u32 current_level = infix_precedence[token.type].current;
        u32 new_level = infix_precedence[token.type].new;

        if (current_level <= level) {
            scanner->current = prev_current;
            break;
        }
        struct Expr *rhs = expression(arena, scanner, new_level);
        lhs = (struct Expr*) make_binary_expr(arena, lhs, rhs, token);
    }
    return lhs;
}

struct Expr *parse(struct Arena *arena, const char *source) {
    struct Scanner scanner;
    init_scanner(&scanner, source);
    struct Expr *expr = expression(arena, &scanner, 0);
    return expr;
}