#include <stdlib.h>
#include "error.h"
#include "ast.h"
#include "parser.h"

struct PrecedenceLevel {
    u32 old;
    u32 new;
};

struct PrecedenceLevel infix_precedence[] = {
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

static struct Token current(struct Parser *parser) {
    return parser->current;
}

static struct Token peek(struct Parser *parser) {
    return parser->next;
}

static struct Token advance(struct Parser *parser) {
    parser->current = parser->next;
    parser->next = next_token(&parser->scanner);
    return parser->current;
}

bool matching_braces(struct Scanner *scanner) {
    struct Token stack[256];
    struct Token token;
    i32 depth = 0;
    while ((token = next_token(scanner)).type != TOKEN_EOF) {
        if (token.type == TOKEN_LEFT_BRACE) {
            stack[depth] = token;
            depth++;
            if (depth == 256) {
                error(token, "exceeded max depth", scanner->source);
                return false;
            }
        } else if (token.type == TOKEN_RIGHT_BRACE ) {
            depth--;
            if (depth < 0) {
                error(token, "unmatched brace", scanner->source);
                return false;
            }
        }
    }
    if (depth > 0) {
        error(stack[depth-1], "unmatched brace", scanner->source);
        return false;
    }
    return true;
}

static void exit_scope(struct Parser *parser) {
    i32 depth = 0;
    enum TokenType type = current(parser).type;
    while (true) {
        if (type == TOKEN_LEFT_BRACE || type == TOKEN_LEFT_PAREN)
            depth++;
        if (type == TOKEN_RIGHT_BRACE || type == TOKEN_RIGHT_PAREN)
            depth--;
        if (depth == -1)
            break;
        type = advance(parser).type;
    }
}

static struct BlockExpr *parse_block_expr(struct Arena *arena, struct Parser *parser);

static struct IfExpr *parse_if_expr(struct Arena *arena, struct Parser *parser);

static struct Node *parse_expr(struct Arena *arena, struct Parser *parser, u32 lvl) {
    struct Node *lhs = NULL;
    struct Token token = advance(parser);

    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_IDENTIFIER:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            lhs = (struct Node*) make_literal_expr(arena, token);    
            break;
        case TOKEN_LEFT_PAREN:
            lhs = (struct Node*) parse_expr(arena, parser, 0);
            if (!lhs)
                return NULL;
            advance(parser);
            break;
        case TOKEN_MINUS:
        case TOKEN_NOT:
            struct Node *rhs = parse_expr(arena, parser, 15);
            if (!rhs)
                return NULL;
            lhs = (struct Node*) make_unary_expr(arena, rhs, token);
            break;
        case TOKEN_IF:
            lhs = (struct Node*) parse_if_expr(arena, parser);
            if (!lhs)
                return NULL;
            break;
        case TOKEN_LEFT_BRACE:
            lhs = (struct Node*) parse_block_expr(arena, parser);
            break;
        default:
            parse_error(parser, token, "expected expression");
            return NULL;
    }
    
    while(true) {
        token = peek(parser);
        u32 old_lvl = infix_precedence[token.type].old;
        u32 new_lvl = infix_precedence[token.type].new;
        if (old_lvl <= lvl) {
            break;
        }
        advance(parser);
        struct Node *rhs = parse_expr(arena, parser, new_lvl);
        if (!rhs) {
            return NULL;
        }
        lhs = (struct Node*) make_binary_expr(arena, lhs, rhs, token);
    }
    return lhs;
}

static struct IfExpr *parse_if_expr(struct Arena *arena, struct Parser *parser) {
    struct Node *test = parse_expr(arena, parser, 0);
    if (!test)
        return NULL;
    
    if (advance(parser).type != TOKEN_LEFT_BRACE) {
        parse_error(parser, peek(parser), "expected `{`");
        return NULL;
    }

    struct BlockExpr *true_block = parse_block_expr(arena, parser);
    if (peek(parser).type != TOKEN_ELSE)
        return make_if_expr(arena, test, true_block, NULL);

    advance(parser);
    if (advance(parser).type != TOKEN_LEFT_BRACE) {
        parse_error(parser, peek(parser), "expected `{`");
        return NULL;
    }

    struct BlockExpr *else_block = parse_block_expr(arena, parser);
    return make_if_expr(arena, test, true_block, else_block);
}

static struct VarStmt *parse_var_stmt(struct Arena *arena, struct Parser *parser) {
    struct Token id = advance(parser);
    if (id.type != TOKEN_IDENTIFIER) {
        parse_error(parser, peek(parser), "expected identifier");
        return NULL;
    }
    switch (advance(parser).type) {
        case TOKEN_SEMICOLON:
            return make_var_stmt(arena, id, NULL);
        case TOKEN_EQUAL:
            struct Node* expr = parse_expr(arena, parser, 0);
            if (!expr)
                return NULL;
            if (peek(parser).type == TOKEN_SEMICOLON)
                advance(parser);
            else
                parse_error(parser, peek(parser), "expected `;`");
            return make_var_stmt(arena, id, expr);
        default:
            parse_error(parser, peek(parser), "expected `=`");
            return NULL;
    }
}

static struct BlockExpr *parse_block_expr(struct Arena *arena, struct Parser *parser) {
    struct NodeList *list = arena_push(arena, sizeof(struct NodeList));
    struct BlockExpr *block = make_block_expr(arena, list);
    struct Node *node;
    enum TokenType type;
    while (true) {
        type = peek(parser).type;
        if (type == TOKEN_RIGHT_BRACE) {
            advance(parser);
            break;
        } else if (type == TOKEN_SEMICOLON) {
            advance(parser);
            continue;
        } else if (type == TOKEN_IF) {
            advance(parser);
            node = (struct Node*) parse_if_expr(arena, parser);
        } else if (type == TOKEN_LEFT_BRACE) {
            advance(parser);
            node = (struct Node*) parse_block_expr(arena, parser);
        }  else if (type == TOKEN_VAR) {
            advance(parser);
            node = (struct Node*) parse_var_stmt(arena, parser);
        } else {
            node = (struct Node*) parse_expr(arena, parser, 0);
            struct Token next = peek(parser);
            if (node && next.type != TOKEN_SEMICOLON && next.type != TOKEN_RIGHT_BRACE)
                parse_error(parser, next, "unexpected token");
        }
        if (!node) {
            exit_scope(parser);
            break;
        }
        if (peek(parser).type == TOKEN_SEMICOLON) {
            advance(parser);
            node = (struct Node*) make_expr_stmt(arena, node);
        }
        list->node = node;
        list->next = arena_push(arena, sizeof(struct NodeList));
        list = list->next;
    }
    return block;
}

struct BlockExpr *parse(struct Arena *arena, const char *source) {
    struct Scanner scanner;
    init_scanner(&scanner, source);
    if (!matching_braces(&scanner))
        return NULL;
    
    init_scanner(&scanner, source);
    struct Parser parser;
    parser.scanner = scanner,
    parser.next = next_token(&parser.scanner),
    parser.had_error = false;
    advance(&parser);
    
    struct BlockExpr *block = parse_block_expr(arena, &parser);
    return parser.had_error ? NULL : block;
}