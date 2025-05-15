#include <string.h>
#include "scan.h"

void init_scanner(struct Scanner *scanner, const char *source) {
    scanner->source = source;
    scanner->start = source;
    scanner->current = source;
}

static char at(struct Scanner *scanner) {
    return scanner->current[0];
}

static bool is_at_end(struct Scanner *scanner) {
    return at(scanner) == '\0';
}

static char next(struct Scanner *scanner) {
    if (is_at_end(scanner))
        return '\0';
    return scanner->current[1];
}

static char bump(struct Scanner *scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static void skip_whitespace(struct Scanner *scanner) {
    while (true) {
        switch (at(scanner)) {
        case '\n':
        case ' ':
        case '\t':
            bump(scanner);
            break;
        default:
            return;
        }
    }
}

static void skip_comment(struct Scanner *scanner) {
    while (at(scanner) != '\n')
        bump(scanner);
    bump(scanner);
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

struct Token mk_token(struct Scanner *scanner, enum TokenTag tag) {
    struct Token token = {
        .span = {.start = scanner->start, .length = (scanner->current - scanner->start)},
        .tag = tag
    };
    return token;
}

struct Token number(struct Scanner *scanner) {
    while(is_digit(at(scanner)))
        bump(scanner);

    if (at(scanner) == '.' && is_digit(next(scanner))) {
        bump(scanner);
        while (is_digit(at(scanner)))
            bump(scanner);
    } 
}

static void identifier(struct Scanner *scanner) {
    while(is_alpha_digit(at(scanner)))
        bump(scanner);
}

static struct Token check_at(struct Scanner *scanner, char c, enum TokenTag tag1, enum TokenTag tag2) {
    if (at(scanner) == c) {
        bump(scanner);
        return mk_token(scanner, tag1);
    }
    return mk_token(scanner, tag2);
}

static struct Token check_keyword(struct Scanner *scanner, const char *rest, u32 length, enum TokenTag tag) {
    u32 token_length = scanner->current - scanner->start;
    if (token_length != length || memcmp(scanner->start+1, rest, length-1) != 0)
        return mk_token(scanner, TOKEN_IDENTIFIER);
    return mk_token(scanner, tag);
}

struct Token next_token(struct Scanner *scanner) {
    skip_whitespace(scanner);
    while (at(scanner) == '/' && next(scanner) == '/') {
        skip_comment(scanner);
        skip_whitespace(scanner);
    }
    scanner->start = scanner->current;

    if (is_at_end(scanner))
        return mk_token(scanner, TOKEN_EOF);

    char c = bump(scanner);
    switch (c) {
    case '+': return check_at(scanner, '=', TOKEN_PLUS_EQ, TOKEN_PLUS);
    case '-': return check_at(scanner, '=', TOKEN_MINUS_EQ, TOKEN_MINUS);
    case '*': return check_at(scanner, '=', TOKEN_STAR_EQ, TOKEN_STAR);
    case '/': return check_at(scanner, '=', TOKEN_SLASH_EQ, TOKEN_SLASH);
    case '<': return check_at(scanner, '=', TOKEN_LEQ, TOKEN_LT);
    case '>': return check_at(scanner, '=', TOKEN_GEQ, TOKEN_GT);
    case '=': return check_at(scanner, '=', TOKEN_EQ_EQ, TOKEN_EQ);
    case '!': return check_at(scanner, '=', TOKEN_NEQ, TOKEN_NOT);
    case '(': return mk_token(scanner, TOKEN_L_PAREN);
    case ')': return mk_token(scanner, TOKEN_R_PAREN);
    case '{': return mk_token(scanner, TOKEN_L_BRACE);
    case '}': return mk_token(scanner, TOKEN_R_BRACE);
    case ';': return mk_token(scanner, TOKEN_SEMI);
    case ',': return mk_token(scanner, TOKEN_COMMA);
    default:
        if (is_digit(c)) {
            number(scanner);
            return mk_token(scanner, TOKEN_NUMBER);
        } else if (is_alpha(c)) {
            identifier(scanner);
            switch (c) {
            case 'a': return check_keyword(scanner, "nd", 3, TOKEN_AND);
            case 'e': return check_keyword(scanner, "lse", 4, TOKEN_ELSE);
            case 'f':
                if (scanner->start[1] == 'n')
                    return check_keyword(scanner, "n", 2, TOKEN_FN);
                else
                    return check_keyword(scanner, "alse", 5, TOKEN_FALSE);
            case 'i': return check_keyword(scanner, "f", 2, TOKEN_IF);
            case 'o': return check_keyword(scanner, "r", 2, TOKEN_OR);
            // TEMP remove when we add functions
            case 'p': return check_keyword(scanner, "rint", 5, TOKEN_PRINT);
            case 'r': return check_keyword(scanner, "eturn", 6, TOKEN_RETURN);
            case 't': return check_keyword(scanner, "rue", 4, TOKEN_TRUE);
            case 'v': return check_keyword(scanner, "ar", 3, TOKEN_VAR);
            default: return mk_token(scanner, TOKEN_IDENTIFIER);
            }
        } else {
            return mk_token(scanner, TOKEN_ERR);
        }
    };
}