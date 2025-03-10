#pragma once
#include "common.h"

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
    i32 length;
};

struct Scanner {
    const char *start;
    const char *current;  
};

void init_scanner(struct Scanner *scanner, const char *source);

struct Token next_token(struct Scanner *scanner);