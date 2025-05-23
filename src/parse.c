#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "parse.h"

struct PrecLvl {
    u32 old;
    u32 new;
};

struct PrecLvl infix_prec[] = {
    [TOKEN_EQ]         = {1, 2},
    [TOKEN_PLUS_EQ]    = {1, 2},
    [TOKEN_MINUS_EQ]   = {1, 2},
    [TOKEN_STAR_EQ]    = {1, 2},
    [TOKEN_SLASH_EQ]   = {1, 2},
    [TOKEN_OR]         = {3, 3},
    [TOKEN_AND]        = {5, 5},
    [TOKEN_EQ_EQ]      = {7, 8},
    [TOKEN_NEQ]        = {7, 8},
    [TOKEN_LT]         = {9, 10},
    [TOKEN_LEQ]        = {9, 10},
    [TOKEN_GT]         = {9, 10},
    [TOKEN_GEQ]        = {9, 10},
    [TOKEN_PLUS]       = {11, 12},
    [TOKEN_MINUS]      = {11, 12},
    [TOKEN_STAR]       = {13, 14},
    [TOKEN_SLASH]      = {13, 14},
    [TOKEN_NOT]        = {0, 0},
    [TOKEN_L_PAREN]    = {0, 0},
    [TOKEN_L_PAREN]    = {0, 0},
    [TOKEN_R_PAREN]    = {0, 0},
    [TOKEN_L_BRACE]    = {0, 0},
    [TOKEN_R_BRACE]    = {0, 0},
    [TOKEN_NUMBER]     = {0, 0},
    [TOKEN_TRUE]       = {0, 0},
    [TOKEN_FALSE]      = {0, 0},
    [TOKEN_IDENTIFIER] = {0, 0},
    [TOKEN_FN]         = {0, 0},
    [TOKEN_RETURN]     = {0, 0},
    [TOKEN_VAR]        = {0, 0},
    [TOKEN_IF]         = {0, 0},
    [TOKEN_ELSE]       = {0, 0},
    [TOKEN_SEMI]       = {0, 0},
    [TOKEN_COLON]      = {0, 0},
    [TOKEN_NUM]        = {0, 0},
    [TOKEN_BOOL]       = {0, 0},
    [TOKEN_EOF]        = {0, 0},
    [TOKEN_ERR]        = {0, 0},
};

void init_file_array(struct FileArr *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->real_paths = allocate(arr->cap * sizeof(const char*));
    arr->bufs = allocate(arr->cap * sizeof(const char*));
    arr->files = allocate(arr->cap * sizeof(struct FileNode*));
}

void release_file_array(struct FileArr *arr) {
    for (i32 i = 0; i < arr->count; i++) {
        release(arr->real_paths[i]);
        release(arr->bufs[i]);
        arr->real_paths[i] = NULL;
        arr->bufs[i] = NULL;
    }
    arr->count = 0;
    arr->cap = 0;
    release(arr->real_paths);
    release(arr->bufs);
    release(arr->files);
    arr->real_paths = NULL;
    arr->bufs = NULL;
    arr->files = NULL;
}

void push_file_array(struct FileArr *arr, const char *real_path, const char *buf, struct FileNode *file) {
    if (arr->count == arr->cap) {
        arr->cap *= 2;
        arr->real_paths = reallocate(arr->real_paths, arr->cap * sizeof(const char*));
        arr->bufs = reallocate(arr->bufs, arr->cap * sizeof(const char*));
        arr->files = reallocate(arr->files, arr->cap * sizeof(struct FileNode*));
    }
    arr->real_paths[arr->count] = real_path;
    arr->bufs[arr->count] = buf;
    arr->files[arr->count] = file;
    arr->count++;
}

void init_parser(struct Parser *parser) {
    init_errlist(&parser->errlist);
    init_arena(&parser->arena);
    parser->panic = false;
}

void release_parser(struct Parser *parser) {
    release_errlist(&parser->errlist);
    release_arena(&parser->arena);
}

// dynarray of Span
struct SpanArray {
    u32 cap;
    u32 count;
    struct Span *spans;
};

static void init_span_array(struct SpanArray *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->spans = allocate(arr->cap * sizeof(struct Span));
}

static void release_span_array(struct SpanArray *arr) {
    arr->count = 0;
    arr->cap = 0;
    release(arr->spans);
    arr->spans = NULL;
}

static void push_span_array(struct SpanArray *arr, struct Span span) {
    if (arr->count == arr->cap) {
        arr->cap *= 2;
        arr->spans = reallocate(arr->spans, arr->cap * sizeof(struct Node*));
    }
    arr->spans[arr->count] = span;
    arr->count++;
}

// moves contents of span array onto arena and release span array
static struct Span *move_span_array_to_arena(struct Arena *arena, struct SpanArray *arr) {
    struct Span *spans = push_arena(arena, arr->count * sizeof(struct Span));
    for (i32 i = 0; i < arr->count; i++)
        spans[i] = arr->spans[i];
    release_span_array(arr);
    return spans;
}

// dynarray of pointers 
struct PtrArray {
    u32 cap;
    u32 count;
    void **ptrs;
};

static void init_ptr_array(struct PtrArray *arr) {
    arr->count = 0;
    arr->cap = 8;
    arr->ptrs = allocate(arr->cap * sizeof(void*));
}

static void release_ptr_array(struct PtrArray *arr) {
    arr->count = 0;
    arr->cap = 0;
    release(arr->ptrs);
    arr->ptrs = NULL;
}

static void push_ptr_array(struct PtrArray *arr, void *ptr) {
    if (arr->count == arr->cap) {
        arr->cap *= 2;
        arr->ptrs = reallocate(arr->ptrs, arr->cap * sizeof(void*));
    }
    arr->ptrs[arr->count] = ptr;
    arr->count++;
}

// moves contents of ptr array onto arena and release ptr array
static void **move_ptr_array_to_arena(struct Arena *arena, struct PtrArray *arr) {
    void **nodes = push_arena(arena, arr->count * sizeof(void*));
    for (i32 i = 0; i < arr->count; i++)
        nodes[i] = arr->ptrs[i];
    release_ptr_array(arr);
    return nodes;
}

static struct LiteralNode *mk_literal(struct Arena *arena, struct Token token) {
    struct LiteralNode *node = push_arena(arena, sizeof(struct LiteralNode));
    node->base.tag = NODE_LITERAL;
    node->base.span = token.span;
    node->lit_tag = token.tag;
    return node;
}

static struct IdentNode *mk_ident(struct Arena *arena, struct Span span) {
    struct IdentNode *node = push_arena(arena, sizeof(struct IdentNode));
    node->base.tag = NODE_IDENT;
    node->base.span = span;
    node->id = -1;
    return node;
}

static struct UnaryNode *mk_unary(struct Arena *arena, struct Token token, struct Node *rhs) {
    struct UnaryNode *node = push_arena(arena, sizeof(struct UnaryNode));
    node->base.tag = NODE_UNARY;
    node->base.span = token.span;
    node->op_tag = token.tag;
    node->rhs = rhs;
    return node;
}

static struct BinaryNode *mk_binary(struct Arena *arena, struct Span span, enum TokenTag tag, struct Node *lhs, struct Node *rhs) {
    struct BinaryNode *node = push_arena(arena, sizeof(struct BinaryNode));
    node->base.tag = NODE_BINARY;
    node->base.span = span;
    node->op_tag = tag;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static struct FnCallNode *mk_fn_call(struct Arena *arena, struct Span span, struct Node *lhs, struct Span *args_spans, struct Node **args, u32 arity) {
    struct FnCallNode *node = push_arena(arena, sizeof(struct FnCallNode));
    node->base.tag = NODE_FN_CALL;
    node->base.span = span;
    node->lhs = lhs;
    node->arg_spans = args_spans;
    node->args = args;
    node->arity = arity;
    return node;
}

static struct VarDeclNode *mk_var_decl(struct Arena *arena, struct Span span, struct TyNode *ty, struct Node *init) {
    struct VarDeclNode *node = push_arena(arena, sizeof(struct VarDeclNode));
    node->base.tag = NODE_VAR_DECL;
    node->base.span = span;
    node->id = -1;
    node->ty_hint = ty;
    node->init = init;
    return node;
}

static struct FnDeclNode *mk_fn_decl(struct Arena *arena, struct Span span, struct FnTyNode *ty, struct BlockNode *body) {
    struct FnDeclNode *node = push_arena(arena, sizeof(struct FnDeclNode));
    node->base.tag = NODE_FN_DECL;
    node->base.span = span;
    node->id = -1;
    node->ty = ty;
    node->body = body;
    return node;
}

static struct ExprStmtNode *mk_expr_stmt(struct Arena *arena, struct Node *expr) {
    struct ExprStmtNode *node = push_arena(arena, sizeof(struct ExprStmtNode));
    node->base.tag = NODE_EXPR_STMT;
    node->base.span.length = 0;
    node->base.span.start = NULL;
    node->expr = expr;
    return node;
}

static struct IfNode *mk_if(struct Arena *arena, struct Span span, struct Node *cond, struct BlockNode *thn, struct BlockNode *els) {
    struct IfNode *node = push_arena(arena, sizeof(struct IfNode));
    node->base.tag = NODE_IF;
    node->base.span = span;
    node->cond = cond;
    node->thn = thn;
    node->els = els;
    return node;
}

static struct BlockNode *mk_block(struct Arena *arena, struct Span span, struct Node **stmts, u32 count) {
    struct BlockNode *node = push_arena(arena, sizeof(struct BlockNode));
    node->base.tag = NODE_BLOCK;
    node->base.span = span;
    node->stmts = stmts;
    node->count = count;
    return node;
}

static struct FileNode *mk_file(struct Arena *arena, struct Node **stmts, struct FileTyNode *ty, u32 count) {
    struct FileNode *node = push_arena(arena, sizeof(struct FileNode));
    node->base.tag = NODE_FILE;
    node->base.span.length = 0;
    node->base.span.start = NULL;
    node->stmts = stmts;
    node->count = count;
    node->ty = ty;
    return node;
}

static struct Token at(struct Parser *parser) {
    return parser->at;
}

static struct Token prev(struct Parser *parser) {
    return parser->prev;
}

static void bump(struct Parser *parser) {
    parser->prev = parser->at;
    parser->at = next_token(&parser->scanner);
}

static bool eat(struct Parser *parser, enum TokenTag tag) {
    if (at(parser).tag == tag) {
        bump(parser);
        return true;
    }
    return false;
}

static bool expect(struct Parser *parser, enum TokenTag tag, const char *msg) {
    if (eat(parser, tag))
        return true;
    if (!parser->panic)
        push_errlist(&parser->errlist, at(parser).span, msg);
    parser->panic = true;
    return false;
}

static void advance_with_err(struct Parser *parser, const char *msg) {
    if (!parser->panic)
        push_errlist(&parser->errlist, at(parser).span, msg);
    bump(parser);
    parser->panic = true;
}

// discard tokens until reach a starting (e.g. var, if) token or exit scope
static void recover_block(struct Parser *parser) {
    parser->panic = false;
    i32 depth = 0;
    while (prev(parser).tag != TOKEN_SEMI && at(parser).tag != TOKEN_EOF) {
        switch(at(parser).tag) {
        case TOKEN_IF:
        case TOKEN_VAR:
            return;
        case TOKEN_L_BRACE:
            depth++;
            break;
        case TOKEN_R_BRACE:
            depth--;
        default:
            break;
        }
        if (depth == -1)
            return;
        bump(parser);
    }
}

static struct BlockNode *parse_block(struct Parser *parser);

static struct Node *parse_expr(struct Parser *parser, u32 prec_lvl) {
    struct Token token = at(parser);
    bump(parser);
    struct Node *lhs = NULL;
    switch (token.tag) {
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NUMBER:
        lhs = (struct Node*)mk_literal(&parser->arena, token);
        break;
    case TOKEN_IDENTIFIER:
        lhs = (struct Node*)mk_ident(&parser->arena, token.span);
        break;
    case TOKEN_L_PAREN:
        lhs = (struct Node*)parse_expr(parser, 1);
        expect(parser, TOKEN_R_PAREN, "expected `)`");
        break;
    case TOKEN_MINUS:
    case TOKEN_NOT:
        lhs = parse_expr(parser, 15);
        lhs = (struct Node*)mk_unary(&parser->arena, token, lhs);
        break;
    default:
        if (!parser->panic)
            push_errlist(&parser->errlist, token.span, "expected expression");
        parser->panic = true;
        return NULL;
    }
    // parse fn calls
    while(eat(parser, TOKEN_L_PAREN)) {
        struct Span fn_call_span = prev(parser).span;
        struct SpanArray scratch_spans;
        struct PtrArray scratch_args;
        init_span_array(&scratch_spans);
        init_ptr_array(&scratch_args);
        while(at(parser).tag != TOKEN_R_PAREN && at(parser).tag != TOKEN_EOF) {
            const char *span_lo = at(parser).span.start;
            struct Node *arg = parse_expr(parser, 1);
            const char *span_hi = prev(parser).span.start + prev(parser).span.length;
            struct Span span = {.start = span_lo, .length = span_hi-span_lo};
            push_span_array(&scratch_spans, span);
            push_ptr_array(&scratch_args, arg);
            if (at(parser).tag != TOKEN_R_PAREN)
                expect(parser, TOKEN_COMMA, "expected `,`");            
        }
        expect(parser, TOKEN_R_PAREN, "expected `)`");
        u32 count = scratch_args.count;
        struct Span *arg_spans = move_span_array_to_arena(&parser->arena, &scratch_spans);
        struct Node **args = move_ptr_array_to_arena(&parser->arena, &scratch_args);
        lhs = (struct Node*)mk_fn_call(&parser->arena, fn_call_span, lhs, arg_spans, args, count);
    }
    while(true) {
        token = at(parser);
        u32 old_lvl = infix_prec[token.tag].old;
        u32 new_lvl = infix_prec[token.tag].new;
        if (old_lvl < prec_lvl) {
            break;
        }
        bump(parser);
        struct Node *rhs = parse_expr(parser, new_lvl);
        
        enum TokenTag tag = token.tag;
        // desugar +=, -=, *=, /=
        switch(tag) {
        case TOKEN_PLUS_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_PLUS, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        case TOKEN_MINUS_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_MINUS, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        case TOKEN_STAR_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_STAR, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        case TOKEN_SLASH_EQ:
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, TOKEN_SLASH, lhs, rhs);
            tag = TOKEN_EQ;
            break;
        }
        lhs = (struct Node*) mk_binary(&parser->arena, token.span, tag, lhs, rhs);
    }
    return lhs;
}

static struct TyNode *parse_ty_expr(struct Parser *parser) {
    struct Token token = at(parser);
    switch(token.tag) {
    case TOKEN_NUM:
        bump(parser);
        return mk_primitive_ty(&parser->arena, TY_NUM);
    case TOKEN_BOOL:
        bump(parser);
        return mk_primitive_ty(&parser->arena, TY_BOOL);
    case TOKEN_ANY:
        bump(parser);
        return mk_primitive_ty(&parser->arena, TY_ANY);
    default:
        if (!parser->panic)
            push_errlist(&parser->errlist, token.span, "expected type");
        parser->panic = true;
        return NULL;
    }
}

// precondition: `if` token consumed
static struct IfNode *parse_if(struct Parser *parser) {
    struct Span span = prev(parser).span;
    expect(parser, TOKEN_L_PAREN, "expected `(`");
    struct Node *cond = parse_expr(parser, 1);
    expect(parser, TOKEN_R_PAREN, "expected `)`");
    struct BlockNode *thn = parse_block(parser);
    struct BlockNode *els = NULL;
    if (eat(parser, TOKEN_ELSE))
        els = parse_block(parser);
    return mk_if(&parser->arena, span, cond, thn, els);
}

// precondition: `var` token consumed
static struct VarDeclNode *parse_var_decl(struct Parser *parser) {
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    struct TyNode *ty_hint = NULL;
    if (eat(parser, TOKEN_COLON))
        ty_hint = parse_ty_expr(parser);
    struct Node *init = NULL;
    if (eat(parser, TOKEN_EQ))
        init = parse_expr(parser, 1);
    expect(parser, TOKEN_SEMI, "expected `;`");
    return mk_var_decl(&parser->arena, span, ty_hint, init);
}

// precondition: `fn` token consumed
static struct FnDeclNode *parse_fn_decl(struct Parser *parser) {
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    expect(parser, TOKEN_L_PAREN, "expected `(`");  
    struct SpanArray scratch_spans;
    struct PtrArray scratch_tys;
    init_span_array(&scratch_spans);
    init_ptr_array(&scratch_tys);
    while(at(parser).tag != TOKEN_R_PAREN && at(parser).tag != TOKEN_EOF) {
        if (eat(parser, TOKEN_IDENTIFIER)) {
            struct Span param_span = prev(parser).span;
            expect(parser, TOKEN_COLON, "expected `:`");
            struct TyNode *ty = parse_ty_expr(parser);
            if (at(parser).tag != TOKEN_R_PAREN)
                expect(parser, TOKEN_COMMA, "expected `,`");
            push_span_array(&scratch_spans, param_span);
            push_ptr_array(&scratch_tys, ty);
        } else {
            advance_with_err(parser, "expected identifier");
        }
    }
    expect(parser, TOKEN_R_PAREN, "expected `)`");
    u32 arity = scratch_spans.count;
    struct Span *param_spans = (struct Span*)move_span_array_to_arena(&parser->arena, &scratch_spans);
    struct TyNode **param_tys = (struct TyNode**)move_ptr_array_to_arena(&parser->arena, &scratch_tys);
    struct TyNode *ret_ty;
    if (at(parser).tag == TOKEN_L_BRACE)
        ret_ty = mk_primitive_ty(&parser->arena, TY_VOID);
    else 
        ret_ty = parse_ty_expr(parser);
    struct FnTyNode *ty = mk_fn_ty(&parser->arena, param_spans, param_tys, ret_ty, arity);
    struct BlockNode *body = parse_block(parser);
    return mk_fn_decl(&parser->arena, span, ty, body);
}

// precondition: `return` token consumed
static struct ReturnNode *parse_return(struct Parser *parser) {
    struct Span span = prev(parser).span;
    struct ReturnNode *node = push_arena(&parser->arena, sizeof(struct ReturnNode));
    node->base.tag = NODE_RETURN;
    node->base.span = span;
    node->expr = NULL;
    if (eat(parser, TOKEN_SEMI))
        return node;
    node->expr = parse_expr(parser, 1);
    expect(parser, TOKEN_SEMI, "expected `;`");
    return node;
}

// TEMP remove when we add functions
// precondition: `print` token consumed
static struct PrintNode *parse_print(struct Parser *parser) {
    struct Span span = prev(parser).span;
    struct Node *expr = parse_expr(parser, 1);
    struct PrintNode *node = push_arena(&parser->arena, sizeof(struct PrintNode));
    node->base.tag = NODE_PRINT;
    node->base.span = span;
    node->expr = expr;
    expect(parser, TOKEN_SEMI, "expected `;`");
    return node;
}

static struct BlockNode *parse_block(struct Parser *parser) {
    struct Span span = at(parser).span;
    if (!expect(parser, TOKEN_L_BRACE, "expected `{`"))
        return NULL;
    struct PtrArray scratch;
    init_ptr_array(&scratch);
    while(at(parser).tag != TOKEN_R_BRACE && at(parser).tag != TOKEN_EOF) {
        struct Node *node;
        if (at(parser).tag == TOKEN_L_BRACE) {
            node = (struct Node*)parse_block(parser);
        } else if (eat(parser, TOKEN_IF)) {
            node = (struct Node*)parse_if(parser);
        } else if (eat(parser, TOKEN_VAR)) {
            node = (struct Node*)parse_var_decl(parser);
        } else if (eat(parser, TOKEN_RETURN)) {
            node = (struct Node*)parse_return(parser);  
        } else if (eat(parser, TOKEN_PRINT)) {
            node = (struct Node*)parse_print(parser);
        } else {
            node = parse_expr(parser, 1);
            expect(parser, TOKEN_SEMI, "expected `;`"); 
            node = (struct Node*) mk_expr_stmt(&parser->arena, node); 
        }
        push_ptr_array(&scratch, node);
        if (parser->panic)
            recover_block(parser);
    }
    expect(parser, TOKEN_R_BRACE, "expected `}`");
    u32 count = scratch.count;
    struct Node **stmts = move_ptr_array_to_arena(&parser->arena, &scratch);
    return mk_block(&parser->arena, span, stmts, count);
}

static struct FileNode *parse_file(struct Parser *parser) {
    struct PropArr prop_arr;
    struct PtrArray scratch;
    init_prop_array(&prop_arr);
    init_ptr_array(&scratch);
    while (at(parser).tag != TOKEN_EOF) {
        // TODO allow global variables
        expect(parser, TOKEN_FN, "expected function declaration");
        struct FnDeclNode *node = parse_fn_decl(parser);
        push_ptr_array(&scratch, (struct Node*)node);
        struct Property prop = {.span = node->base.span, .ty = (struct TyNode*)node->ty};
        push_prop_array(&prop_arr, prop);
    }
    u32 count = scratch.count;
    struct Node **stmts = move_ptr_array_to_arena(&parser->arena, &scratch);
    u32 prop_count = prop_arr.count;
    struct Property *props = move_prop_array_to_arena(&parser->arena, &prop_arr);
    struct FileTyNode *ty = mk_file_ty(&parser->arena, props, count);
    return mk_file(&parser->arena, stmts, ty, count); // FIXME!
}

// TODO disallow trailing comma in function declarations and calls
void parse(struct Parser *parser, struct FileArr *arr, const char *path) {
    // TODO when we add imports this will become DFS
    const char *real_path = realpath(path, NULL);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        printf("File `%s` does not exist\n", path);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    u64 length = ftell(fp); 
    fseek(fp, 0, SEEK_SET);
    char *buf = allocate(length+2);
    if (!buf) {
        printf("Failed to allocate buffer for file `%s`\n", path);
        exit(1);
    }
    buf[0] = '\0';
    buf[length+1] = '\0';
    char *source = buf+1;
    fread(source, 1, length, fp);

    init_scanner(&parser->scanner, source);
    parser->at = next_token(&parser->scanner);
    parser->panic = false;

    struct FileNode *file = parse_file(parser);
    push_file_array(arr, real_path, buf, file);
    fclose(fp);
}