#pragma once
#include "common.h"
#include "arena.h"

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

    TOKEN_NULL,
    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,

    // TODO handle these keywords
    TOKEN_IMPORT,
    TOKEN_AS,

    TOKEN_FN,
    TOKEN_CLASS,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_SEMI,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,

    TOKEN_EOF,
    TOKEN_ERR,

    // TEMP remove when we add functions
    TOKEN_PRINT
};

struct Span {
    const char *start;
    i32 len;
    i32 line;
};

struct Token {
    struct Span span;
    enum TokenTag tag;
};

struct ErrMsg {
    struct Span span;
    const char *msg;
};

struct ErrList {
    i32 cnt;
    i32 cap;
    struct ErrMsg *errs;
};

struct Parser {
    struct ErrList errlist;
    struct Arena arena;
    const char *source;
    const char *start;
    const char *current;
    i32 line;
    struct Token at;
    struct Token prev;
    bool panic;
};

struct Token next_token(struct Parser *parser);