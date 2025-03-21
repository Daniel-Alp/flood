#include <stdio.h>
#include "error.h"
#include "scanner.h"

void error(struct Token token, const char *message, const char *source) {
    const char *line_start = token.start;
    while (line_start != source && *(line_start-1) != '\n') {
        line_start--;
    }
    const char *line_end = token.start;
    while (*line_end != '\0' && *line_end != '\n') {
        line_end++;
    }

    printf("Line %d\n", token.line);
    u32 line_length = line_end - line_start;
    printf("    %.*s\n", line_length, line_start);
    u32 offset = token.start - line_start;
    printf("    %*s^\n", offset, "");
    printf("Error: %s\n", message);
}