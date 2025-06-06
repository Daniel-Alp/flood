#pragma once
#include "common.h"

enum TokenTag {
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_SLASH_SLASH,

    TOKEN_LT,
    TOKEN_LEQ,
    TOKEN_GT,
    TOKEN_GEQ,
    TOKEN_EQEQ,
    TOKEN_NEQ,

    TOKEN_EQ,
    TOKEN_PLUS_EQ,
    TOKEN_MINUS_EQ,
    TOKEN_STAR_EQ,
    TOKEN_SLASH_EQ,
    TOKEN_PERCENT_EQ,
    TOKEN_SLASH_SLASH_EQ,

    TOKEN_AND,
    TOKEN_OR,
    
    TOKEN_NOT,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,
    TOKEN_L_SQUARE,
    TOKEN_R_SQUARE,

    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,

    // TODO handle these keywords
    TOKEN_IMPORT,
    TOKEN_AS,

    TOKEN_FN,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_SEMI,
    TOKEN_COMMA,
    TOKEN_DOT,

    TOKEN_EOF,
    TOKEN_ERR,

    // TEMP remove when we add functions
    TOKEN_PRINT
};

struct Scanner {
    const char *source;
    const char *start;
    const char *current;
    u32 line;
};

struct Span {
    const char *start;
    i32 length;
    u32 line;
};

struct Token {
    struct Span span;
    enum TokenTag tag;
};

void init_scanner(struct Scanner *scanner, const char *source);

struct Token next_token(struct Scanner *scanner);