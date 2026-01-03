#include <string.h>
#include "scan.h"
#include "error.h"

static bool is_digit(const char c) 
{
    return '0' <= c && c <= '9'; 
}

static bool is_alpha(const char c) 
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alpha_digit(const char c) 
{
    return is_digit(c) || is_alpha(c);
}

Scanner::~Scanner()
{
    source = nullptr;
    start = nullptr;
    current = nullptr;
    line = 0;
}

char Scanner::at() const
{
    return current[0];
}

bool Scanner::is_at_end() const
{
    return at() == '\0';
}

char Scanner::next() const
{
    if (is_at_end())
        return '\0';
    return current[1];
}

char Scanner::bump()
{
    current++;
    return current[-1];
}

void Scanner::skip_whitespace()
{
    while (true) {
        switch (at()) {
        case '\n':
            line++;
        case ' ':
        case '\t':
            bump();
            break;
        default:
            return;
        }
    }
}

void Scanner::skip_comment()
{
    while (at() != '\n' && !is_at_end())
        bump();
    if (at() == '\n') {
        bump();
        line++;
    }
}

Token Scanner::mk_token(TokenTag tag) const
{
    const struct Token token = {
        .span = {
            .start = start, 
            .len = i32(current - start),
            .line = line
        },
        .tag = tag,
    };
    return token;
}

Token Scanner::number()
{
    while(is_digit(at()))
        bump();

    if (at() == '.' && is_digit(next())) {
        bump();
        while (is_digit(at()))
            bump();
    } 
    return mk_token(TOKEN_NUMBER);
}

Token Scanner::string()
{
    while(at() != '"') {
        bump();
        // TODO remove outdated TODO's >:)
        // TODO support multi-line strings zig style
        // TODO not increasing line if string contains newline, this is bad
        // TODO meaningful error message during parsing, not just "expected expression"
        if (is_at_end()) {
            Token token = mk_token(TOKEN_ERR);
            errarr_.push(ErrMsg{.span = token.span, .msg = "unterminated string"});
            return token;
        }
    }
    bump();
    const struct Token token = {
        .span = {
            .start = start+1, 
            .len = i32(current - start - 2),
            .line = line
        },
        .tag = TOKEN_STRING,
    };
    return token;
}

Token Scanner::check_at(const char c, TokenTag tagthen, TokenTag tagelse)
{
    if (at() == c) {
        bump();
        return mk_token(tagthen);
    }
    return mk_token(tagelse);
}

Token Scanner::check_keyword(const char *rest, const i32 len, TokenTag tag)
{
    while(is_alpha_digit(at()))
        bump();
    const i32 token_len = current - start;
    if (token_len != len || memcmp(start+1, rest, len-1) != 0)
        return mk_token(TOKEN_IDENTIFIER);
    return mk_token(tag);
}

Token Scanner::next_token() 
{
    skip_whitespace();
    while (at() == '#') {
        skip_comment();
        skip_whitespace();
    }
    start = current;

    if (is_at_end())
        return mk_token(TOKEN_EOF);

    const char c = bump();
    switch (c) {
    case '+': return check_at('=', TOKEN_PLUS_EQ, TOKEN_PLUS);
    case '-': return check_at('=', TOKEN_MINUS_EQ, TOKEN_MINUS);
    case '*': return check_at('=', TOKEN_STAR_EQ, TOKEN_STAR);
    case '/': {
        if (at() == '/') {
            bump();
            return check_at('=', TOKEN_SLASH_SLASH_EQ, TOKEN_SLASH_SLASH);   
        } else {
            return check_at('=', TOKEN_SLASH_EQ, TOKEN_SLASH);
        }
    }
    case '%': return check_at('=', TOKEN_PERCENT_EQ, TOKEN_PERCENT);
    case '<': return check_at('=', TOKEN_LEQ, TOKEN_LT);
    case '>': return check_at('=', TOKEN_GEQ, TOKEN_GT);
    case '=': return check_at('=', TOKEN_EQEQ, TOKEN_EQ);
    case '!': return check_at('=', TOKEN_NEQ, TOKEN_NOT);
    case '(': return mk_token(TOKEN_L_PAREN);
    case ')': return mk_token(TOKEN_R_PAREN);
    case '{': return mk_token(TOKEN_L_BRACE);
    case '}': return mk_token(TOKEN_R_BRACE);
    case '[': return mk_token(TOKEN_L_SQUARE);
    case ']': return mk_token(TOKEN_R_SQUARE);
    case ';': return mk_token(TOKEN_SEMI);
    case ',': return mk_token(TOKEN_COMMA);
    case '.': return mk_token(TOKEN_DOT);
    case ':': return mk_token(TOKEN_COLON);
    case '"': return string();
    default:
        if (is_digit(c)) {
            return number();
        } else if (is_alpha(c)) {
            switch (c) {
            case 'a': 
                if (next() == 's')
                    return check_keyword("s", 2, TOKEN_AS);
                else
                    return check_keyword("nd", 3, TOKEN_AND);
            case 'e': return check_keyword("lse", 4, TOKEN_ELSE);
            case 'c': return check_keyword("lass", 5, TOKEN_CLASS);
            case 'f':
                if (next() == 'n')
                    return check_keyword("n", 2, TOKEN_FN);
                else
                    return check_keyword("alse", 5, TOKEN_FALSE);
            case 'i': 
                if (next() == 'f')
                    return check_keyword("f", 2, TOKEN_IF);
                else
                    return check_keyword("mport", 6, TOKEN_IMPORT);
            case 'n': return check_keyword("ull", 4, TOKEN_NULL);
            case 'o': return check_keyword("r", 2, TOKEN_OR);
            // TEMP remove when we add functions
            case 'p': return check_keyword("rint", 5, TOKEN_PRINT);
            case 'r': return check_keyword("eturn", 6, TOKEN_RETURN);
            case 't': return check_keyword("rue", 4, TOKEN_TRUE);
            case 'v': return check_keyword("ar", 3, TOKEN_VAR);
            default: {
                while(is_alpha_digit(at()))
                    bump();
                return mk_token(TOKEN_IDENTIFIER);
            }
            }
        } else {
            struct Token token = mk_token(TOKEN_ERR);
            errarr_.push(ErrMsg{.span = token.span, .msg = "unexpected token"});
            return token;
        }
    };
}

// static char at(const struct Parser *parser) 
// {
//     return parser->current[0];
// }

// static bool is_at_end(const struct Parser *parser) 
// {
//     return at(parser) == '\0';
// }

// static char next(const struct Parser *parser) 
// {
//     if (is_at_end(parser))
//         return '\0';
//     return parser->current[1];
// }

// static char bump(struct Parser *parser) 
// {
//     parser->current++;
//     return parser->current[-1];
// }

// static void skip_whitespace(struct Parser *parser) 
// {
//     while (true) {
//         switch (at(parser)) {
//         case '\n':
//             parser->line++;
//             fall through
//         case ' ':
//         case '\t':
//             bump(parser);
//             break;
//         default:
//             return;
//         }
//     }
// }

// static void skip_comment(struct Parser *parser) 
// {
//     while (at(parser) != '\n' && !is_at_end(parser))
//         bump(parser);
//     if (at(parser) == '\n') {
//         bump(parser);
//         parser->line++;
//     }
// }

// static bool is_digit(const char c) 
// {
//     return '0' <= c && c <= '9'; 
// }

// static bool is_alpha(const char c) 
// {
//     return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
// }

// static bool is_alpha_digit(const char c) 
// {
//     return is_digit(c) || is_alpha(c);
// }

// static struct Token mk_token(const struct Parser *parser, const enum TokenTag tag) 
// {
//     struct Token token = {
//         .span = {
//             .start = parser->start, 
//             .len = (parser->current - parser->start),
//             .line = parser->line
//         },
//         .tag = tag,
//     };
//     return token;
// }

// static struct Token number(struct Parser *parser) 
// {
//     while(is_digit(at(parser)))
//         bump(parser);

//     if (at(parser) == '.' && is_digit(next(parser))) {
//         bump(parser);
//         while (is_digit(at(parser)))
//             bump(parser);
//     } 
//     return mk_token(parser, TOKEN_NUMBER);
// }

// static struct Token string(struct Parser *parser)
// {
//     while(at(parser) != '"') {
//         bump(parser);
//         TODO support multi-line strings zig style
//         TODO meaningful error message during parsing, not just "expected expression"
//         if (is_at_end(parser)) {
//             struct Token token = mk_token(parser, TOKEN_ERR);
//             push_errarr(&parser->errarr, token.span, "unterminated string");
//             return token;
//         }
//     }
//     bump(parser);
//     struct Token token = {
//         .span = {
//             .start = parser->start+1, 
//             .len = (parser->current - parser->start - 2),
//             .line = parser->line
//         },
//         .tag = TOKEN_STRING,
//     };
//     return token;
// }

// static void identifier(struct Parser *parser) 
// {
//     while(is_alpha_digit(at(parser)))
//         bump(parser);
// }

// static struct Token check_at(struct Parser *parser, char c, enum TokenTag tag1, enum TokenTag tag2) 
// {
//     if (at(parser) == c) {
//         bump(parser);
//         return mk_token(parser, tag1);
//     }
//     return mk_token(parser, tag2);
// }

// static struct Token check_keyword(struct Parser *parser, const char *rest, i32 len, enum TokenTag tag) 
// {
//     i32 token_len = parser->current - parser->start;
//     if (token_len != len || memcmp(parser->start+1, rest, len-1) != 0)
//         return mk_token(parser, TOKEN_IDENTIFIER);
//     return mk_token(parser, tag);
// }

// struct Token next_token(struct Parser *parser) 
// {
//     skip_whitespace(parser);
//     while (at(parser) == '#') {
//         skip_comment(parser);
//         skip_whitespace(parser);
//     }
//     parser->start = parser->current;

//     if (is_at_end(parser))
//         return mk_token(parser, TOKEN_EOF);

//     const char c = bump(parser);
//     switch (c) {
//     case '+': return check_at(parser, '=', TOKEN_PLUS_EQ, TOKEN_PLUS);
//     case '-': return check_at(parser, '=', TOKEN_MINUS_EQ, TOKEN_MINUS);
//     case '*': return check_at(parser, '=', TOKEN_STAR_EQ, TOKEN_STAR);
//     case '/': {
//         if (at(parser) == '/') {
//             bump(parser);
//             return check_at(parser, '=', TOKEN_SLASH_SLASH_EQ, TOKEN_SLASH_SLASH);   
//         } else {
//             return check_at(parser, '=', TOKEN_SLASH_EQ, TOKEN_SLASH);
//         }
//     }
//     case '%': return check_at(parser, '=', TOKEN_PERCENT_EQ, TOKEN_PERCENT);
//     case '<': return check_at(parser, '=', TOKEN_LEQ, TOKEN_LT);
//     case '>': return check_at(parser, '=', TOKEN_GEQ, TOKEN_GT);
//     case '=': return check_at(parser, '=', TOKEN_EQEQ, TOKEN_EQ);
//     case '!': return check_at(parser, '=', TOKEN_NEQ, TOKEN_NOT);
//     case '(': return mk_token(parser, TOKEN_L_PAREN);
//     case ')': return mk_token(parser, TOKEN_R_PAREN);
//     case '{': return mk_token(parser, TOKEN_L_BRACE);
//     case '}': return mk_token(parser, TOKEN_R_BRACE);
//     case '[': return mk_token(parser, TOKEN_L_SQUARE);
//     case ']': return mk_token(parser, TOKEN_R_SQUARE);
//     case ';': return mk_token(parser, TOKEN_SEMI);
//     case ',': return mk_token(parser, TOKEN_COMMA);
//     case '.': return mk_token(parser, TOKEN_DOT);
//     case ':': return mk_token(parser, TOKEN_COLON);
//     case '"': return string(parser);
//     default:
//         if (is_digit(c)) {
//             return number(parser);
//         } else if (is_alpha(c)) {
//             identifier(parser);
//             switch (c) {
//             case 'a': 
//                 if (parser->start[1] == 's')
//                     return check_keyword(parser, "s", 2, TOKEN_AS);
//                 else
//                     return check_keyword(parser, "nd", 3, TOKEN_AND);
//             case 'e': return check_keyword(parser, "lse", 4, TOKEN_ELSE);
//             case 'c': return check_keyword(parser, "lass", 5, TOKEN_CLASS);
//             case 'f':
//                 if (parser->start[1] == 'n')
//                     return check_keyword(parser, "n", 2, TOKEN_FN);
//                 else
//                     return check_keyword(parser, "alse", 5, TOKEN_FALSE);
//             case 'i': 
//                 if (parser->start[1] == 'f')
//                     return check_keyword(parser, "f", 2, TOKEN_IF);
//                 else
//                     return check_keyword(parser, "mport", 6, TOKEN_IMPORT);
//             case 'n': return check_keyword(parser, "ull", 4, TOKEN_NULL);
//             case 'o': return check_keyword(parser, "r", 2, TOKEN_OR);
//             TEMP remove when we add functions
//             case 'p': return check_keyword(parser, "rint", 5, TOKEN_PRINT);
//             case 'r': return check_keyword(parser, "eturn", 6, TOKEN_RETURN);
//             case 't': return check_keyword(parser, "rue", 4, TOKEN_TRUE);
//             case 'v': return check_keyword(parser, "ar", 3, TOKEN_VAR);
//             default: return mk_token(parser, TOKEN_IDENTIFIER);
//             }
//         } else {
//             struct Token token = mk_token(parser, TOKEN_ERR);
//             push_errarr(&parser->errarr, token.span, "unexpected token");
//             return token;
//         }
//     };
// }
