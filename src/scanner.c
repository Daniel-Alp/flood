#include <stdbool.h>
#include "scanner.h"

struct Scanner scanner;

void init_scanner(const char* src) {
    scanner.start = src;
    scanner.current = src;
}

bool is_at_end() {
    return peek() == '\0';
}

char peek() {
    return scanner.current[0];
}

char peek_next() {
    if (is_at_end()) {
        return '\0';
    }
    return scanner.current[1];
}

char advance() {
    if (is_at_end()) {
        return '\0';
    }
    scanner.current++;
    return scanner.current[-1];
}

void skip_ws() {
    char c;
    while (true) {
        c = peek();
        switch(c) {
            case ' ':
            case '\t':
            case '\n':
                scanner.current++;
                break;
            default:
                return;
        }
    }    
}

struct Token make_token(enum TokenType type) {
    struct Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = scanner.current - scanner.start;
    return token;
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

struct Token make_number() {
    while (is_digit(peek())) {
        advance();
    }
    if (peek() == '.' && is_digit(peek_next())) {
        advance();
        while (is_digit(peek())) {
            advance();       
        }
    }
    return make_token(TOKEN_NUMBER);
}

struct Token next_token() {
    skip_ws();
    scanner.start = scanner.current;
    char next = advance();
    switch (next) {
        case '+': 
            return make_token(TOKEN_PLUS);
        case '-':
            return make_token(TOKEN_MINUS);
        case '*':
            return make_token(TOKEN_STAR);
        case '/':
            return make_token(TOKEN_SLASH);
        case '(':
            return make_token(TOKEN_LEFT_PAREN);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '\0':
            return make_token(TOKEN_EOF);
        default:
            if (is_digit(next)) {
                return make_number();
            } else {
                return make_token(TOKEN_ERROR);
            }
    }
}