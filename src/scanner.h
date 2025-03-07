#pragma once
#include <stdbool.h>

enum TokenType {
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_NUMBER,
    TOKEN_EOF,
    TOKEN_ERROR
};

struct Token {
    enum TokenType type;
    const char *start;
    int length;
};

struct Scanner {
    const char *start;
    const char *current;
};

void init_scanner(const char *src);

bool is_at_end();

char peek();

char peek_next();

char advance();

void skip_ws();

struct Token make_token(enum TokenType type);

bool is_digit(char c);

struct Token make_number();

struct Token next_token();