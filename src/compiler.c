#include <stdlib.h>
#include "../util/memory.h"
#include "compiler.h"

#include "debug.h"

void compile_expr(struct Expr *expr, struct Chunk *chunk);

void compile_unary(struct UnaryExpr *expr, struct Chunk *chunk) {
    compile_expr(expr->rhs, chunk);
    switch (expr->op.type) {
        case TOKEN_MINUS:
            write_chunk(chunk, OP_NEGATE);
            break;
    }
}

void compile_binary(struct BinaryExpr *expr, struct Chunk *chunk) {
    compile_expr(expr->lhs, chunk);
    compile_expr(expr->rhs, chunk);
    switch (expr->op.type) {
        case TOKEN_PLUS:
            write_chunk(chunk, OP_ADD);
            break;
        case TOKEN_MINUS:
            write_chunk(chunk, OP_SUB);
            break;
        case TOKEN_STAR:
            write_chunk(chunk, OP_MUL);
            break;
        case TOKEN_SLASH:
            write_chunk(chunk, OP_DIV);
            break;
    }
}

void compile_literal(struct LiteralExpr *expr, struct Chunk *chunk) {
    struct Token token = expr->value;
    double val = strtod(token.start, NULL);
    write_chunk(chunk, OP_CONST);
    write_value_array(&chunk->constants, val);
    write_chunk(chunk, (&chunk->constants)->count-1);
}

void compile_expr(struct Expr *expr, struct Chunk *chunk) {
    switch (expr->type) {
        case EXPR_UNARY:
            compile_unary((struct UnaryExpr*) expr, chunk);
            break;
        case EXPR_BINARY:
            compile_binary((struct BinaryExpr*) expr, chunk);
            break;
        case EXPR_LITERAL:
            compile_literal((struct LiteralExpr*) expr, chunk);
            break;
    }
}

void compile(struct Chunk *chunk, struct Expr *expr){
    compile_expr(expr, chunk);
    write_chunk(chunk, OP_RETURN);
}