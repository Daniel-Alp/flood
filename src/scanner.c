#include <string.h>
#include "scanner.h"

void init_scanner(struct Scanner *scanner, const char *source) {
    scanner->source = source;
    scanner->current = source;
    scanner->start = source;
    scanner->line = 1;
}

static char peek(struct Scanner *scanner) {
    return scanner->current[0];
}

static bool is_at_end(struct Scanner *scanner) {
    return peek(scanner) == '\0';
}

static char peek_next(struct Scanner *scanner) {
    if (is_at_end(scanner)) {
        return '\0';
    }
    return scanner->current[1];
}

static char advance(struct Scanner *scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static void skip_whitespace(struct Scanner *scanner) {
    while (true) {
        char c = peek(scanner);
        switch (c) {
            case ' ':
            case '\t':
                scanner->current++;
                break;
            case '\n':
                scanner->current++;
                scanner->line++;
                break;
            default:
                return;
        }
    }
}

struct Token make_token(struct Scanner *scanner, enum TokenType type) {
    struct Token token = {
        .type = type,
        .start = scanner->start,
        .length = (scanner->current - scanner->start),
        .line = scanner->line,
    };
    return token;
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

static void number(struct Scanner *scanner) {
    while(is_digit(peek(scanner))) {
        advance(scanner);
    }
    if (peek(scanner) == '.' && is_digit(peek_next(scanner))) {
        advance(scanner);
        while (is_digit(peek(scanner))) {
            advance(scanner);
        }
    }
}

static void identifier(struct Scanner *scanner) {
    while(is_alpha_digit(peek(scanner))) {
        advance(scanner);
    }
}

static struct Token check_keyword(struct Scanner *scanner, const char *rest, u32 length, enum TokenType type) {
    u32 token_length = scanner->current - scanner->start;
    if (token_length != length || memcmp(scanner->start+1, rest, length-1)) {
        return make_token(scanner, TOKEN_IDENTIFIER);
    }
    return make_token(scanner, type);
}

struct Token next_token(struct Scanner *scanner) {
    skip_whitespace(scanner);
    scanner->start = scanner->current;
    if (is_at_end(scanner)) {
        return make_token(scanner, TOKEN_EOF);
    }
    char c = advance(scanner);
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
        case '=': return make_token(scanner, TOKEN_EQUAL); // Change to check for ==
        default:
            if (is_digit(c)) {
                number(scanner);
                return make_token(scanner, TOKEN_NUMBER);
            } else if (is_alpha(c)) {
                identifier(scanner);
                switch (c) {
                    case 'l': return check_keyword(scanner, "et", 3, TOKEN_LET);
                    case 'v': return check_keyword(scanner, "ar", 3, TOKEN_VAR);
                    default:  return make_token(scanner, TOKEN_IDENTIFIER);
                }
            } else {
                return make_token(scanner, TOKEN_ERROR);
            }
    };
}