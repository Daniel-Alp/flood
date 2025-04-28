#include <stdio.h>
#include "debug.h"
#include "error.h"
#include "ty.h"

// n != 0
static u32 line_digits(u32 n) {
    u32 c = 0;
    while(n) {
        c++;
        n /= 10;
    }
    return c;
}

// TODO 
// create an array where the index is the line number and the value is a pointer 
// to the start of the line in the source code. Then lookup can be done using binary search
static u32 line_num(const char *pos) {
    u32 line = 1;
    while (*pos) {
        if (*pos == '\n')
            line++;
        pos--;
    }
    return line;
}

static u32 get_line_indent(const char *pos) {
    const char *start = pos;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    return pos - start;
}

static void print_pipe(u32 max_n) {
    u32 pad = line_digits(max_n);
    printf("%*s | ", pad, "");
}

// When printing several lines, we want to align them, e.g.
// ```
//      9  |
//      10 |
// ```
// max_n is the max line number that we will be printing.
static void print_line_num(u32 n, u32 max_n) {
    u32 pad = line_digits(max_n) - line_digits(n);
    printf("%d%*s | ", n, pad, "");
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

// print a region of source code 
// if span starts and ends on one line
// ```     
//        |
//     LL | <code>
//        | ^^^^^^
// ```
// if span starts and ends on diff lines
// ```
//        |
//     LL |      <code lo>
//        |  ____^
//     LL | |          <code hi>
//        | |__________^
// ```
static void print_span(const char *span_lo, const char *span_hi) {
    u32 lo_line = line_num(span_lo);
    u32 hi_line = line_num(span_hi);
    print_pipe(hi_line);
    printf("\n");

    print_line_num(lo_line, hi_line);
    u32 lo_indent = get_line_indent(span_lo);
    if (lo_line == hi_line) {
        print_line(span_lo);

        print_pipe(hi_line);
        printf("%*s", lo_indent, "");
        for (i32 i = 0; i < span_hi - span_lo + 1; i++)
            printf("^");
        printf(" ");
    } else {
        printf(" ");
        print_line(span_lo);

        print_pipe(hi_line);
        printf(" ");
        for (i32 i = 0; i < lo_indent; i++)
            printf("_");
        printf("^\n");

        print_line_num(hi_line, hi_line);
        printf("|");
        print_line(span_hi);

        print_pipe(hi_line);
        printf("|");
        u32 hi_indent = get_line_indent(span_hi);
        for (i32 i = 0; i < hi_indent; i++)
            printf("_");
        printf("^ ");
    }
}

void emit_parse_error(struct Parser *parser, const char *msg) {
    if (parser->panic)
        return;
    parser->had_error = true;
    parser->panic = true;
    print_span(parser->at.start, parser->at.start + parser->at.length-1);
    printf("%s\n\n", msg);
}

void emit_resolver_error(struct Span span, const char *msg, struct Resolver *resolver) {
    resolver->had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("%s\n\n", msg);
}

void emit_ty_error_unary(struct Span span, struct Ty *ty, bool *had_error) {
    if (ty_head(ty) == TY_ERR)
        return;
    *had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("cannot apply `%.*s` to ", span.length, span.start);
    print_ty(ty);
    printf("\n\n");
}

void emit_ty_error_binary(struct Span span, struct Ty *ty_lhs, struct Ty *ty_rhs, bool *had_error) {
    if (ty_head(ty_lhs) == TY_ERR || ty_head(ty_rhs) == TY_ERR)
        return;
    *had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("cannot apply `%.*s` to ", span.length, span.start);
    print_ty(ty_lhs);
    printf(" and ");
    print_ty(ty_rhs);
    printf("\n\n");
}

void emit_ty_error_if(struct Span span, struct Ty *ty_thn, struct Ty *ty_els, bool *had_error) {
    if (ty_head(ty_thn) == TY_ERR || ty_head(ty_els) == TY_ERR)
        return;
    *had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("`if` and `else` have incompatible types. ");
    print_ty(ty_thn);
    printf(" and ");
    print_ty(ty_els);
    printf("\n\n");    
}

void emit_ty_error_if_cond(struct Span span, struct Ty *ty_cond, bool *had_error) {
    if (ty_head(ty_cond) == TY_ERR)
        return;
    *had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("expected `Bool` found ");
    print_ty(ty_cond);
    printf("\n\n");
}

void emit_ty_error_uninitialized(struct Span span, bool *had_error) {
    *had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("`%.*s` used here before initialized\n\n", span.length, span.start);
}

void emit_ty_error_invalid_assignment(struct Span span, bool *had_error) {
    *had_error = true;
    print_span(span.start, span.start + span.length-1);
    printf("cannot assign to this expression\n\n", span.length, span.start);
}

void emit_ty_error_cannot_infer(struct Span span) {
    print_span(span.start, span.start + span.length-1);
    printf("cannot infer type\n\n");
}