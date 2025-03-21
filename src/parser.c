#include "error.h"
#include "parser.h"

bool matching_braces(struct Scanner *scanner) {
    struct Token stack[256];
    i32 stack_top = 0;
    
    struct Token token;
    while ((token = next_token(scanner)).type != TOKEN_EOF) {
        if (token.type == TOKEN_LEFT_BRACE) {
            stack[stack_top] = token;
            stack_top++;
            if (stack_top == 256) {
                error(token, "exceeded 256 layers of nesting", scanner->source);
                return false;
            }
        } else if (token.type == TOKEN_RIGHT_BRACE) {
            stack_top--;
            if (stack_top < 0) {
                error(token, "unmatched '}'", scanner->source);
                return false;
            }
        }
    }
    
    if (stack_top > 0) {
        error(stack[stack_top-1], "unmatched '{'", scanner->source);
        return false;
    }
    
    return true;
}