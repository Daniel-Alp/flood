#include "scanner.h"

void init_scanner(struct Scanner *scanner, const char *source) {
    scanner->current = source;
    scanner->start = source;
    scanner->line = 1;
    scanner->offset = 0;
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
    scanner->offset++;
    return scanner->current[-1];
}

static void skip_whitespace(struct Scanner *scanner) {
    while (true) {
        char c = peek(scanner);
        switch (c) {
            case ' ':
            case '\t':
                scanner->current++;
                scanner->offset++;
                break;
            case '\n':
                scanner->current++;
                scanner->offset = 0;
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
        .offset = scanner->offset
    };
    return token;
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

struct Token make_number(struct Scanner *scanner) {
    while(is_digit(peek(scanner))) {
        advance(scanner);
    }
    if (peek(scanner) == '.') {
        advance(scanner);
        while (is_digit(peek(scanner))) {
            advance(scanner);
        }
    }
    return make_token(scanner, TOKEN_NUMBER);
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
        default:
            if (is_digit(c)) {
                return make_number(scanner);
            } else {
                return make_token(scanner, TOKEN_ERROR);
            }
    };
}