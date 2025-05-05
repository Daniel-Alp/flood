#pragma once
#include "common.h"

enum TokenTag {
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_EQ,
    TOKEN_PLUS_EQ,
    TOKEN_MINUS_EQ,
    TOKEN_STAR_EQ,
    TOKEN_SLASH_EQ,

    TOKEN_LT,
    TOKEN_LEQ,
    TOKEN_GT,
    TOKEN_GEQ,
    TOKEN_EQ_EQ,
    TOKEN_NEQ,

    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,

    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,

    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_SEMI,

    TOKEN_COLON,
    TOKEN_TY_NUM,
    TOKEN_TY_BOOL,

    TOKEN_EOF,
    TOKEN_ERR,

    // TEMP remove when we add functions
    TOKEN_PRINT
};

struct Scanner {
    const char *source;
    const char *start;
    const char *current;
};

struct Span {
    const char *start;
    i32 length;
};

struct Token {
    struct Span span;
    enum TokenTag tag;
};

void init_scanner(struct Scanner *scanner, const char *source);

struct Token next_token(struct Scanner *scanner);