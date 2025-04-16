#include <stdio.h>
#include "error.h"

void emit_error(struct Token token, const char *msg, const char *source) {
    const char *line_start = token.start;
    while (line_start != source && line_start[-1] != '\n')
        line_start--;

    const char *line_end = token.start;
    while (*line_end != '\0' && *line_end != '\n')
        line_end++;

    printf("Line %d\n", token.line);
    u32 line_length = line_end - line_start;
    printf("    %.*s\n", line_length, line_start);
    u32 offset = token.start - line_start;
    printf("    %*s^\n", offset, "");
    printf("Error: %s\n", msg);
}

void emit_parse_error(struct Token token, const char *msg, struct Parser *parser) {
    emit_error(token, msg, parser->scanner.source);
    parser->had_error = true;
}