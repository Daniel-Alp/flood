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

// lines are computed lazily
// TODO 
// create an array where the index is the line number and the value is a pointer 
// to the start of the line in the source code. Then lookup can be done using binary search
static u32 get_line_num(const char *start) {
    u32 line = 1;
    while (*start) {
        if (*start == '\n')
            line++;
        start--;
    }
    return line;
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

static void print_line(const char *offset) {
    const char *start = offset;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    const char *end = offset;
    while (*end != '\0' && *end != '\n')
        end++;
    u32 length = end-start;
    printf("%.*s\n", length, start);
}

static void print_up_arrow(const char *offset, bool underscore) {
    const char *start = offset;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    u32 indent = offset - start;
    if (underscore) {
        for (i32 i = 0; i < indent; i++)
            printf("_");
        printf("^ ");
    } else {
        printf("%*s^ ", indent, "");
    }
}

static void print_underline(const char *offset, u32 length) {
    const char *start = offset;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    u32 indent = offset - start;
    printf("%*s", indent, "");
    for (i32 i = 0; i < length; i++)
        printf("^");
    printf(" ");
}

void emit_error(struct Token token, const char *msg) {
    u32 line = get_line_num(token.start);
    print_pipe(line);
    printf("\n");

    print_line_num(line, line);
    print_line(token.start);

    print_pipe(line);
    print_underline(token.start, token.length);
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
    u32 line = get_line_num(token.start);
    print_pipe(line);
    printf("\n");

    print_line_num(line, line);
    print_line(token.start);

    print_pipe(line);
    print_up_arrow(token.start, false);
    printf("cannot apply `%.*s` to ", token.length, token.start);
    print_ty(ty);
    printf("\n\n");
}

void emit_ty_error_binary(struct Token token, struct Ty *ty_lhs, struct Ty *ty_rhs, bool *had_error) {
    if (ty_head(ty_lhs) == TY_ERR || ty_head(ty_rhs) == TY_ERR)
        return;
    *had_error = true;
    u32 line = get_line_num(token.start);
    print_pipe(line);
    printf("\n");

    print_line_num(line, line);
    print_line(token.start);

    print_pipe(line);
    print_up_arrow(token.start, false);
    printf("cannot apply `%.*s` to ", token.length, token.start);
    print_ty(ty_lhs);
    printf(" and ");
    print_ty(ty_rhs);
    printf("\n\n");
}

void emit_ty_error_if(struct Span span, struct Ty *ty_thn, struct Ty *ty_els, bool *had_error) {
    if (ty_head(ty_thn) == TY_ERR || ty_head(ty_els) == TY_ERR)
        return;
    *had_error = true;
    u32 line = get_line_num(span.start);
    print_pipe(line);
    printf("\n");

    print_line_num(line, line);
    print_line(span.start);

    print_pipe(line);
    print_up_arrow(span.start, false);
    printf("`if` and `else` have incompatible types. ");
    print_ty(ty_els);
    printf(" and ");
    print_ty(ty_thn);
    printf("\n\n");    
}

void emit_ty_error_if_cond(struct Span span, struct Ty *ty_cond, bool *had_error) {
    if (ty_head(ty_cond) == TY_ERR)
        return;
    *had_error = true;
    u32 line_start = get_line_num(span.start);
    u32 line_end = get_line_num(span.start + span.length);
    print_pipe(line_end);
    printf("\n");

    // if line_start == line_end prints error message like this
    //      if 3 + 4 {
    //         ^^^^^ expected `Bool` found `Num`
    // otherwise, prints error message line this
    //      if 3 +
    //  _______^
    // |       4
    // |_______^ expected `Bool` found `Num`
    if (line_start == line_end) {
        print_line_num(line_start, line_end);
        print_line(span.start);

        print_pipe(line_end);
        print_underline(span.start, span.length);
        printf("expected `Bool` found ");
        print_ty(ty_cond);
        printf("\n\n");
    } else {
        print_line_num(line_start, line_end);
        printf(" ");
        print_line(span.start);

        print_pipe(line_end);
        printf(" ");
        print_up_arrow(span.start, true);
        printf("\n");

        print_line_num(line_end, line_end);
        printf("|");
        print_line(span.start + span.length);

        print_pipe(line_end);
        printf("|");
        print_up_arrow(span.start + span.length-1, true);
        printf("expected `Bool` found ");
        print_ty(ty_cond);
        printf("\n\n");
    }
}

void emit_ty_error_uninitialized(struct Token token, bool *had_error) {
    *had_error = true;
    u32 line = get_line_num(token.start);
    print_pipe(line);
    printf("\n");

    print_line_num(line, line);
    print_line(token.start);

    print_pipe(line);
    print_underline(token.start, token.length);
    printf("`%.*s` used here before initialized\n\n", token.length, token.start);
}