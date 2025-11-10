#include <string.h>
#include "scan.h"
#include "error.h"

static char at(struct Parser *parser) 
{
    return parser->current[0];
}

static bool is_at_end(struct Parser *parser) 
{
    return at(parser) == '\0';
}

static char next(struct Parser *parser) 
{
    if (is_at_end(parser))
        return '\0';
    return parser->current[1];
}

static char bump(struct Parser *parser) 
{
    parser->current++;
    return parser->current[-1];
}

static void skip_whitespace(struct Parser *parser) 
{
    while (true) {
        switch (at(parser)) {
        case '\n':
            parser->line++;
            // fall through
        case ' ':
        case '\t':
            bump(parser);
            break;
        default:
            return;
        }
    }
}

static void skip_comment(struct Parser *parser) 
{
    while (at(parser) != '\n' && !is_at_end(parser))
        bump(parser);
    if (at(parser) == '\n') {
        bump(parser);
        parser->line++;
    }
}

static bool is_digit(char c) 
{
    return '0' <= c && c <= '9'; 
}

static bool is_alpha(char c) 
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alpha_digit(char c) 
{
    return is_digit(c) || is_alpha(c);
}

static struct Token mk_token(struct Parser *parser, enum TokenTag tag) 
{
    struct Token token = {
        .span = {
            .start = parser->start, 
            .len = (parser->current - parser->start),
            .line = parser->line
        },
        .tag = tag,
    };
    return token;
}

static struct Token number(struct Parser *parser) 
{
    while(is_digit(at(parser)))
        bump(parser);

    if (at(parser) == '.' && is_digit(next(parser))) {
        bump(parser);
        while (is_digit(at(parser)))
            bump(parser);
    } 
    return mk_token(parser, TOKEN_NUMBER);
}

static struct Token string(struct Parser *parser)
{
    while(at(parser) != '"') {
        bump(parser);
        // TODO support multi-line strings zig style
        // TODO meaningful error message during parsing, not just "expected expression"
        if (is_at_end(parser)) {
            struct Token token = mk_token(parser, TOKEN_ERR);
            push_errlist(&parser->errlist, token.span, "unterminated string");
            return token;
        }
    }
    bump(parser);
    struct Token token = {
        .span = {
            .start = parser->start+1, 
            .len = (parser->current - parser->start - 2),
            .line = parser->line
        },
        .tag = TOKEN_STRING,
    };
    return token;
}

static void identifier(struct Parser *parser) 
{
    while(is_alpha_digit(at(parser)))
        bump(parser);
}

static struct Token check_at(struct Parser *parser, char c, enum TokenTag tag1, enum TokenTag tag2) 
{
    if (at(parser) == c) {
        bump(parser);
        return mk_token(parser, tag1);
    }
    return mk_token(parser, tag2);
}

static struct Token check_keyword(struct Parser *parser, const char *rest, i32 len, enum TokenTag tag) 
{
    i32 token_len = parser->current - parser->start;
    if (token_len != len || memcmp(parser->start+1, rest, len-1) != 0)
        return mk_token(parser, TOKEN_IDENTIFIER);
    return mk_token(parser, tag);
}

struct Token next_token(struct Parser *parser) 
{
    skip_whitespace(parser);
    while (at(parser) == '#') {
        skip_comment(parser);
        skip_whitespace(parser);
    }
    parser->start = parser->current;

    if (is_at_end(parser))
        return mk_token(parser, TOKEN_EOF);

    char c = bump(parser);
    switch (c) {
    case '+': return check_at(parser, '=', TOKEN_PLUS_EQ, TOKEN_PLUS);
    case '-': return check_at(parser, '=', TOKEN_MINUS_EQ, TOKEN_MINUS);
    case '*': return check_at(parser, '=', TOKEN_STAR_EQ, TOKEN_STAR);
    case '/': {
        if (at(parser) == '/') {
            bump(parser);
            return check_at(parser, '=', TOKEN_SLASH_SLASH_EQ, TOKEN_SLASH_SLASH);   
        } else {
            return check_at(parser, '=', TOKEN_SLASH_EQ, TOKEN_SLASH);
        }
    }
    case '%': return check_at(parser, '=', TOKEN_PERCENT_EQ, TOKEN_PERCENT);
    case '<': return check_at(parser, '=', TOKEN_LEQ, TOKEN_LT);
    case '>': return check_at(parser, '=', TOKEN_GEQ, TOKEN_GT);
    case '=': return check_at(parser, '=', TOKEN_EQEQ, TOKEN_EQ);
    case '!': return check_at(parser, '=', TOKEN_NEQ, TOKEN_NOT);
    case '(': return mk_token(parser, TOKEN_L_PAREN);
    case ')': return mk_token(parser, TOKEN_R_PAREN);
    case '{': return mk_token(parser, TOKEN_L_BRACE);
    case '}': return mk_token(parser, TOKEN_R_BRACE);
    case '[': return mk_token(parser, TOKEN_L_SQUARE);
    case ']': return mk_token(parser, TOKEN_R_SQUARE);
    case ';': return mk_token(parser, TOKEN_SEMI);
    case ',': return mk_token(parser, TOKEN_COMMA);
    case '.': return mk_token(parser, TOKEN_DOT);
    case ':': return mk_token(parser, TOKEN_COLON);
    case '"': return string(parser);
    default:
        if (is_digit(c)) {
            return number(parser);
        } else if (is_alpha(c)) {
            identifier(parser);
            switch (c) {
            case 'a': 
                if (parser->start[1] == 's')
                    return check_keyword(parser, "s", 2, TOKEN_AS);
                else
                    return check_keyword(parser, "nd", 3, TOKEN_AND);
            case 'e': return check_keyword(parser, "lse", 4, TOKEN_ELSE);
            case 'c': return check_keyword(parser, "lass", 5, TOKEN_CLASS);
            case 'f':
                if (parser->start[1] == 'n')
                    return check_keyword(parser, "n", 2, TOKEN_FN);
                else
                    return check_keyword(parser, "alse", 5, TOKEN_FALSE);
            case 'i': 
                if (parser->start[1] == 'f')
                    return check_keyword(parser, "f", 2, TOKEN_IF);
                else
                    return check_keyword(parser, "mport", 6, TOKEN_IMPORT);
            case 'n': return check_keyword(parser, "ull", 4, TOKEN_NULL);
            case 'o': return check_keyword(parser, "r", 2, TOKEN_OR);
            // TEMP remove when we add functions
            case 'p': return check_keyword(parser, "rint", 5, TOKEN_PRINT);
            case 'r': return check_keyword(parser, "eturn", 6, TOKEN_RETURN);
            case 't': return check_keyword(parser, "rue", 4, TOKEN_TRUE);
            case 'v': return check_keyword(parser, "ar", 3, TOKEN_VAR);
            default: return mk_token(parser, TOKEN_IDENTIFIER);
            }
        } else {
            struct Token token = mk_token(parser, TOKEN_ERR);
            push_errlist(&parser->errlist, token.span, "unexpected token");
            return token;
        }
    };
}