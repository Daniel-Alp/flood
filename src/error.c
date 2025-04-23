#include <stdio.h>
#include "error.h"

void emit_error(struct Token token, const char *msg) {
    const char *line_start = token.start;
    while (line_start[-1] != '\0' && line_start[-1] != '\n')
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