#pragma once
#include "common.h"

enum TokenKind {
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LEQ,
    TOKEN_GEQ,
    TOKEN_EQEQ,
    TOKEN_NEQ,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_SEMICOLON,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_VAR,
    TOKEN_EOF,
    TOKEN_ERROR,
};

struct Token {
    const char *start;
    i32 length;
    enum TokenKind kind;
};

struct Scanner {
    const char *start;
    const char *current;
};

void init_scanner(struct Scanner *scanner, const char *source);

struct Token next_token(struct Scanner *scanner);