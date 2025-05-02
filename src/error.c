#include <stdio.h>
#include "error.h"

// n != 0
static u32 num_digits(u32 n) {
    u32 c = 0;
    while(n) {
        c++;
        n /= 10;
    }
    return c;
}

static u32 line_num(const char *ptr) {
    u32 line = 1;
    while (*ptr) {
        if (*ptr == '\n')
            line++;
        ptr--;
    }
    return line;
}

static u32 line_indent(const char *ptr) {
    const char *start = ptr;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    return ptr - start;
}

static void print_line(const char *ptr) {
    const char *start = ptr;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    const char *end = ptr;
    while (*end != '\0' && *end != '\n')
        end++;
    u32 length = end-start;
    printf("%.*s\n", length, start);
}

void print_parse_errs(struct ParseErrList *errlist) {
    for (i32 i = 0; i < errlist->count; i++) {
        struct ParseErr err = errlist->errs[i];
        
        u32 line = line_num(err.span.start);
        u32 pad = num_digits(line_num(err.span.start));
        
        printf("%d | ", line);
        print_line(err.span.start);

        printf("%*s | ", pad, "");
        printf("%*s", line_indent(err.span.start), "");
        printf("^");
        printf(" %s\n\n", err.msg);
    }
}