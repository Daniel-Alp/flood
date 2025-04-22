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

static struct LiteralNode *mk_literal(struct Arena *arena, struct Token val) {
    struct LiteralNode *node = arena_push(arena, sizeof(struct LiteralNode));
    node->base.type = NODE_LITERAL;
    node->val = val;
    return node;
}

static struct VariableNode *mk_variable(struct Arena *arena, struct Token name) {
    struct VariableNode *node = arena_push(arena, sizeof(struct LiteralNode));
    node->base.type = NODE_VARIABLE;
    node->name = name;
    node->id = 0;
    return node;
}

struct UnaryNode *mk_unary(struct Arena *arena, struct Node *rhs, struct Token op) {
    struct UnaryNode *node = arena_push(arena, sizeof(struct UnaryNode));
    node->base.type = NODE_UNARY;
    node->rhs = rhs;
    node->op = op;
    return node;
}

struct BinaryNode *mk_binary(struct Arena *arena, struct Node *lhs, struct Node *rhs, struct Token op) {
    struct BinaryNode *expr = arena_push(arena, sizeof(struct BinaryNode));
    expr->base.type = NODE_BINARY;
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = op;
    return expr;
}

static struct IfNode *mk_if(struct Arena *arena, struct Node *cond, struct BlockNode *thn, struct BlockNode *els) {
    struct IfNode *node = arena_push(arena, sizeof(struct IfNode));
    node->base.type = NODE_IF;
    node->cond = cond;
    node->thn = thn;
    node->els = els;
    return node;
}

static struct VarDeclNode *mk_var_decl(struct Arena *arena, struct VariableNode *var, struct Node *init) {
    struct VarDeclNode *stmt = arena_push(arena, sizeof(struct VarDeclNode));
    stmt->base.type = NODE_VAR_DECL;
    stmt->var = var;
    stmt->init = init;
    return stmt;
}

static struct BlockNode *mk_block(struct Arena *arena, struct NodeList *stmts) {
    struct BlockNode *node = arena_push(arena, sizeof(struct BlockNode));
    node->base.type = NODE_BLOCK;
    node->stmts = stmts;
    return node;
}

static struct ExprStmtNode *mk_expr_stmt(struct Arena *arena, struct Node *expr) {
    struct ExprStmtNode *node = arena_push(arena, sizeof(struct ExprStmtNode));
    node->base.type = NODE_EXPR_STMT;
    node->expr = expr;
    return node;
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
    emit_parse_error(parser, msg);
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

static struct IfNode *parse_if(struct Arena *arena, struct Parser *parser);

static struct BlockNode *parse_block(struct Arena *arena, struct Parser *parser);

static struct VarDeclNode *parse_var_decl(struct Arena *arena, struct Parser *parser);

static struct Node *parse_expr(struct Arena *arena, struct Parser *parser, u32 prec_lvl) {
    if (parser->panic)
        return NULL;
    struct Node *lhs = NULL;
    struct Token token = at(parser);
    switch (token.type) {
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NUMBER:
            bump(parser);
            lhs = (struct Node*)mk_literal(arena, token);
            break;
        case TOKEN_IDENTIFIER:
            bump(parser);
            lhs = (struct Node*)mk_variable(arena, token);
            break;
        case TOKEN_LEFT_PAREN:
            bump(parser);
            lhs = parse_expr(arena, parser, 0);
            expect(parser, TOKEN_RIGHT_PAREN, "expected `)`");
            break;
        case TOKEN_MINUS:
        case TOKEN_NOT:
            bump(parser);
            lhs = (struct Node*)mk_unary(arena, parse_expr(arena, parser, 15), token);
            break;
        case TOKEN_IF:
            lhs = (struct Node*)parse_if(arena, parser);
            break;
        case TOKEN_LEFT_BRACE:
            lhs = (struct Node*)parse_block(arena, parser);
            break;
        default:
            emit_parse_error(parser, "unexpected token");
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
        lhs = (struct Node*) mk_binary(arena, lhs, rhs, token);
    }
    return lhs;
}

static struct BlockNode *parse_block(struct Arena *arena, struct Parser *parser) {
    if (parser->panic)
        return NULL;
    expect(parser, TOKEN_LEFT_BRACE, "expected `{`");
    struct NodeList *stmts = arena_push(arena, sizeof(struct NodeList));
    struct BlockNode *block = mk_block(arena, stmts);
    while (!eat(parser, TOKEN_RIGHT_BRACE)) {
        enum TokenType type = at(parser).type;
        struct Node *node;
        if (type == TOKEN_EOF) {
            emit_parse_error(parser, "unexpected EOF");
            break;
        } else if (type == TOKEN_SEMICOLON) {
            bump(parser);
            continue;
        } else if (type == TOKEN_LEFT_BRACE) {
            node = (struct Node*) parse_block(arena, parser);
        } else if (type == TOKEN_IF) {
            node = (struct Node*) parse_if(arena, parser);
        } else if (type == TOKEN_VAR) {
            node = (struct Node*) parse_var_decl(arena, parser);
        } else {
            node = parse_expr(arena, parser, 0);
            if (!parser->panic && at(parser).type != TOKEN_RIGHT_BRACE) {
                expect(parser, TOKEN_SEMICOLON, "expected `;`");
                parser->panic = false;
                node = (struct Node*)mk_expr_stmt(arena, node);
            }   
        }
        if (parser->panic) {
            recover(parser);
            parser->panic = false;
        }
        stmts->node = node;
        stmts->next = arena_push(arena, sizeof(struct NodeList));
        stmts = stmts->next;
    }
    return block;
}

static struct IfNode *parse_if(struct Arena *arena, struct Parser *parser) {
    if (parser->panic)
        return NULL;
    expect(parser, TOKEN_IF, "expected `if`");
    struct Node *cond = parse_expr(arena, parser, 0);
    struct BlockNode *thn = parse_block(arena, parser);
    struct BlockNode *els = NULL;
    if (eat(parser, TOKEN_ELSE))
        els = parse_block(arena, parser);
    return mk_if(arena, cond, thn, els);
}

static struct VariableNode *parse_variable(struct Arena *arena, struct Parser *parser) {
    if (parser->panic)
        return NULL;
    struct Token name = at(parser);
    if (name.type != TOKEN_IDENTIFIER)
        emit_parse_error(parser, "expected identifier");
    else
        bump(parser);
    return mk_variable(arena, name);
}

static struct VarDeclNode *parse_var_decl(struct Arena *arena, struct Parser *parser) {
    if (parser->panic)
        return NULL;
    expect(parser, TOKEN_VAR, "expected `var`");
    struct VariableNode *var = parse_variable(arena, parser);
    struct Node *init = NULL;
    if (eat(parser, TOKEN_EQUAL))
        init = parse_expr(arena, parser, 0);
    expect(parser, TOKEN_SEMICOLON, "expected `;`");
    return mk_var_decl(arena, var, init);
}

struct BlockNode *parse(struct Arena *arena, const char *source) {
    struct Scanner scanner;
    init_scanner(&scanner, source);
    struct Token at = next_token(&scanner);
    struct Parser parser = {
        .scanner = scanner,
        .at = at, 
        .had_error = false,
        .panic = false
    };
    struct BlockNode *block = parse_block(arena, &parser);
    return parser.had_error ? NULL : block;
}