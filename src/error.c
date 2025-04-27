#include <stdio.h>
#include "debug.h"
#include "error.h"
#include "ty.h"

static u32 num_digits(u32 n) {
    if (n == 0)
        return 1;
    u32 c = 0;
    while(n) {
        c++;
        n /= 10;
    }
    return c;
}

// When printing several lines, we want to align them, e.g.
//      9  |
//      10 |
// max_n is the max line number that we will be printing.
static void print_line_num(u32 n, u32 max_n) {
    u32 pad = num_digits(max_n) - num_digits(n);
    printf("%d%*s | ", n, pad, "");
}

static void print_pipe(u32 max_n) {
    u32 pad = num_digits(max_n);
    printf("%*s | ", pad, "");
}

static void print_line(struct Token token) {
    const char *start = token.start;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    const char *end = token.start;
    while (*end != '\0' && *end != '\n')
        end++;
    u32 length = end-start;
    printf("%.*s", length, start);
}

static void print_up_arrow(struct Token token) {
    const char *start = token.start;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    u32 offset = token.start - start;
    printf("%*s^ ", offset, "");
}

void emit_error(struct Token token, const char *msg) {
    print_pipe(token.line);
    printf("\n");

    print_line_num(token.line, token.line);
    print_line(token);
    printf("\n");

    print_pipe(token.line);
    print_up_arrow(token);
    printf("%s\n\n", msg);
}

void emit_parse_error(struct Parser *parser, const char *msg) {
    if (!parser->panic)
        emit_error(parser->at, msg);
    parser->had_error = true;
    parser->panic = true;
}

void emit_resolver_error(struct Resolver *resolver, struct Token token, const char *msg) {
    resolver->had_error = true;
    emit_error(token, msg);
}

void emit_ty_error_unary(struct Token token, struct Ty *ty, bool *had_error) {
    if (ty_head(ty) == TY_ERR)
        return;
    *had_error = true;
    print_pipe(token.line);
    printf("\n");

    print_line_num(token.line, token.line);
    print_line(token);
    printf("\n");

    print_pipe(token.line);
    print_up_arrow(token);
    printf("cannot apply `%.*s` to ", token.length, token.start);
    print_ty(ty);
    printf("\n\n");
}

void emit_ty_error_binary(struct Token token, struct Ty *ty_lhs, struct Ty *ty_rhs, bool *had_error) {
    if (ty_head(ty_lhs) == TY_ERR || ty_head(ty_rhs) == TY_ERR)
        return;
    *had_error = true;
    print_pipe(token.line);
    printf("\n");

    print_line_num(token.line, token.line);
    print_line(token);

    printf("\n");
    print_pipe(token.line);
    print_up_arrow(token);
    printf("cannot apply `%.*s` to ", token.length, token.start);
    print_ty(ty_lhs);
    printf(" and ");
    print_ty(ty_rhs);
    printf("\n\n");
}

void emit_ty_error_if(bool *had_error) {
    // TODO improve error message
    *had_error = true;
    printf("`if` and `else` have incompatible types\n");
}