#include <stdio.h>
#include "error.h"

void error(struct Parser *parser, const char *message) {
    if (parser->panic) {
        return;
    }
    parser->panic = true;
    parser->had_error = true;
    
    struct Token token = parser->current;
    const char *source = parser->scanner.source;
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
    printf("SyntaxError: %s\n", message);
};