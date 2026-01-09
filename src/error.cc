#include "error.h"
#include "dynarr.h"
#include <stdio.h>

// n != 0
static i32 num_digits(i32 n)
{
    i32 c = 0;
    while (n) {
        c++;
        n /= 10;
    }
    return c;
}

static i32 line_num(const char *ptr)
{
    i32 line = 1;
    // error at EOF
    if (*ptr == '\0')
        ptr--;
    while (*ptr) {
        if (*ptr == '\n')
            line++;
        ptr--;
    }
    return line;
}

static i32 print_indent(const char *ptr)
{
    const char *start = ptr;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    const i32 indent = ptr - start;
    printf("%*s", indent, "");
    return indent;
}

static i32 print_line(const char *ptr)
{
    const char *start = ptr;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    const char *end = ptr;
    while (*end != '\0' && *end != '\n')
        end++;
    const i32 length = end - start;
    printf("%.*s\n", length, start);
    return length;
}

#define ANSI_COLOR_BLUE  "\x1b[34m"
#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_BOLD        "\e[1m"
#define ANSI_BOLD_RESET  "\e[m"

// precondition: at least one error
void print_errarr(const Dynarr<ErrMsg> &errarr, const bool color)
{
    const i32 last_line = line_num(errarr[errarr.len() - 1].span.start);
    const i32 max_pad = num_digits(last_line);

    for (i32 i = 0; i < errarr.len(); i++) {
        const ErrMsg err = errarr[i];

        const i32 line = line_num(err.span.start);
        const i32 pad = num_digits(line);

        if (color)
            printf(ANSI_COLOR_BLUE ANSI_BOLD);
        printf("%d%*s | ", line, max_pad - pad, "");
        if (color)
            printf(ANSI_COLOR_RESET ANSI_BOLD_RESET);
        const i32 length = print_line(err.span.start);

        if (color)
            printf(ANSI_COLOR_BLUE ANSI_BOLD);
        printf("%*s | ", max_pad, "");
        if (color)
            printf(ANSI_COLOR_RESET ANSI_BOLD_RESET);
        if (color)
            printf(ANSI_COLOR_RED ANSI_BOLD);
        const i32 indent = print_indent(err.span.start);
        printf("^");
        for (i32 i = 0; i < err.span.len - 1 && i < length - 1 - indent; i++)
            printf("~");
        printf(" %s\n\n", err.msg);
        if (color)
            printf(ANSI_BOLD_RESET ANSI_COLOR_RESET);
    }
}
