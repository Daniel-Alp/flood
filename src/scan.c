#include <string.h>
#include "scan.h"

void init_scanner(struct Scanner *scanner, const char *source) {
    scanner->current = source;
    scanner->start = source;
}

static char at(struct Scanner *scanner) {
    return scanner->current[0];
}

static bool is_at_end(struct Scanner *scanner) {
    return at(scanner) == '\0';
}

static char next(struct Scanner *scanner) {
    if (is_at_end(scanner))
        return '\0';
    return scanner->current[1];
}

static char bump(struct Scanner *scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static void skip_whitespace(struct Scanner *scanner) {
    while (true) {
        char c = at(scanner);
        switch (c) {
            case '\n':
            case ' ':
            case '\t':
                bump(scanner);
                break;
            default:
                return;
        }
    }
}

static bool is_digit(char c) {
    return '0' <= c && c <= '9'; 
}

static bool is_alpha(char c) {
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || c == '_';
}

static bool is_alpha_digit(char c) {
    return is_digit(c) || is_alpha(c);
}

struct Token make_token(struct Scanner *scanner, enum TokenKind kind) {
    struct Token token = {
        .kind = kind,
        .start = scanner->start,
        .length = (scanner->current - scanner->start),
    };
    return token;
}

static void number(struct Scanner *scanner) {
    while(is_digit(at(scanner)))
        bump(scanner);

    if (at(scanner) == '.' && is_digit(next(scanner))) {
        bump(scanner);
        while (is_digit(at(scanner)))
            bump(scanner);
    }
}

static void identifier(struct Scanner *scanner) {
    while(is_alpha_digit(at(scanner)))
        bump(scanner);
}

static struct Token check_next(struct Scanner *scanner, char next, enum TokenKind kind1, enum TokenKind kind2) {
    if (at(scanner) == next) {
        bump(scanner);
        return make_token(scanner, kind1);
    }
    return make_token(scanner, kind2);
}

static struct Token check_keyword(struct Scanner *scanner, const char *rest, u32 length, enum TokenKind kind) {
    u32 token_length = scanner->current - scanner->start;
    if (token_length != length || memcmp(scanner->start+1, rest, length-1) != 0)
        return make_token(scanner, TOKEN_IDENTIFIER);
    return make_token(scanner, kind);
}

struct Token next_token(struct Scanner *scanner) {
    skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (is_at_end(scanner))
        return make_token(scanner, TOKEN_EOF);

    char c = bump(scanner);
    switch (c) {
        case '+': return make_token(scanner, TOKEN_PLUS);
        case '-': return make_token(scanner, TOKEN_MINUS);
        case '*': return make_token(scanner, TOKEN_STAR);
        case '/': return make_token(scanner, TOKEN_SLASH);
        case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
        case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
        case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
        case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
        case ';': return make_token(scanner, TOKEN_SEMICOLON);
        case '=': return check_next(scanner, '=', TOKEN_EQUAL_EQUAL, TOKEN_EQUAL);
        case '<': return check_next(scanner, '=', TOKEN_LESS_EQUAL, TOKEN_LESS);
        case '>': return check_next(scanner, '=', TOKEN_GREATER, TOKEN_LESS);
        case '&': return check_next(scanner, '&', TOKEN_AND, TOKEN_ERROR);
        case '|': return check_next(scanner, '|', TOKEN_OR, TOKEN_ERROR);
        case '!': return check_next(scanner, '=', TOKEN_NOT_EQUAL, TOKEN_NOT);
        default:
            if (is_digit(c)) {
                number(scanner);
                return make_token(scanner, TOKEN_NUMBER);
            } else if (is_alpha(c)) {
                identifier(scanner);
                switch (c) {
                    case 'v': return check_keyword(scanner, "ar", 3, TOKEN_VAR);
                    case 'i': return check_keyword(scanner, "f", 2, TOKEN_IF);
                    case 'e': return check_keyword(scanner, "lse", 4, TOKEN_ELSE);
                    case 't': return check_keyword(scanner, "rue", 4, TOKEN_TRUE);
                    case 'f': return check_keyword(scanner, "alse", 5, TOKEN_FALSE);
                    default:  return make_token(scanner, TOKEN_IDENTIFIER);
                }
            } else {
                return make_token(scanner, TOKEN_ERROR);
            }
    };
}