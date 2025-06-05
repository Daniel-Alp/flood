#include <stdio.h>
#include "memory.h"
#include "error.h"

void init_errlist(struct ErrList *errlist) 
{
    errlist->cnt = 0;
    errlist->cap = 8;
    errlist->errs = allocate(errlist->cap * sizeof(struct ErrMsg));
}

void release_errlist(struct ErrList *errlist) 
{
    errlist->cnt = 0;
    errlist->cap = 0;
    release(errlist->errs);
    errlist->errs = NULL;
}

void push_errlist(struct ErrList *errlist, struct Span span, const char *msg) 
{
    struct ErrMsg err = {.span = span, .msg = msg};
    if (errlist->cnt == errlist->cap) {
        errlist->cap *= 2;
        errlist->errs = reallocate(errlist->errs, errlist->cap * sizeof(struct ErrMsg));
    }
    errlist->errs[errlist->cnt] = err;
    errlist->cnt++;
}

// n != 0
static u32 num_digits(u32 n) 
{
    u32 c = 0;
    while(n) {
        c++;
        n /= 10;
    }
    return c;
}

static u32 line_num(const char *ptr) 
{
    u32 line = 1;
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

static u32 line_indent(const char *ptr) 
{
    const char *start = ptr;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    return ptr - start;
}

static void print_line(const char *ptr) 
{
    const char *start = ptr;
    while (start[-1] != '\0' && start[-1] != '\n')
        start--;
    const char *end = ptr;
    while (*end != '\0' && *end != '\n')
        end++;
    u32 length = end-start;
    printf("%.*s\n", length, start);
}

#define ANSI_COLOR_BLUE  "\x1b[34m"
#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_BOLD        "\e[1m"
#define ANSI_BOLD_RESET  "\e[m"

// precondition: at least one error
void print_errlist(struct ErrList *errlist) 
{

    u32 last_line = line_num(errlist->errs[errlist->cnt-1].span.start);
    u32 max_pad = num_digits(last_line);

    for (i32 i = 0; i < errlist->cnt; i++) {
        struct ErrMsg err = errlist->errs[i];
        
        u32 line = line_num(err.span.start);
        u32 pad = num_digits(line);
        
        printf(ANSI_COLOR_BLUE ANSI_BOLD "%d%*s | " ANSI_COLOR_RESET ANSI_BOLD_RESET, 
            line, max_pad - pad, "");
        print_line(err.span.start);

        printf(ANSI_COLOR_BLUE ANSI_BOLD "%*s | %*s" ANSI_COLOR_RESET ANSI_BOLD_RESET, 
            max_pad, "", line_indent(err.span.start), "");
        // TODO handle multi-line spans properly
        printf(ANSI_COLOR_RED ANSI_BOLD "^");
        for (i32 i = 0; i < err.span.length - 1; i++)
            printf("~");
        printf(" %s\n\n" ANSI_BOLD_RESET ANSI_COLOR_RESET, err.msg);
    }
}