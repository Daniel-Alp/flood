#pragma once
#include "common.h"

enum TokenType {
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_EQUAL,
    TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_GREATER, TOKEN_EQUAL_EQUAL, 
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_NOT_EQUAL,
    TOKEN_NUMBER, TOKEN_IDENTIFIER, TOKEN_TRUE, TOKEN_FALSE, 
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_SEMICOLON,
    TOKEN_IF, TOKEN_ELSE,
    TOKEN_VAR,
    TOKEN_EOF,
    TOKEN_ERROR
};

struct Token {
    enum TokenType type;
    const char *start;
    i32 length;    
    i32 line;
};

struct Scanner {
    const char *start;
    const char *current;  
    const char *source;
    i32 line;
};

void init_scanner(struct Scanner *scanner, const char *source);

struct Token next_token(struct Scanner *scanner);