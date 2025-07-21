#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "parse.h"

void init_parser(struct Parser *parser) 
{
    init_errlist(&parser->errlist);
    init_arena(&parser->arena);
    parser->panic = false;
}

void release_parser(struct Parser *parser) 
{
    release_errlist(&parser->errlist);
    release_arena(&parser->arena);
}

struct PtrArray {
    u32 cap;
    u32 cnt;
    void **ptrs;
};

static void init_ptr_array(struct PtrArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 8;
    arr->ptrs = allocate(arr->cap * sizeof(void*));
}

static void release_ptr_array(struct PtrArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 0;
    release(arr->ptrs);
    arr->ptrs = NULL;
}

static void push_ptr_array(struct PtrArray *arr, void *ptr) 
{
    if (arr->cnt == arr->cap) {
        arr->cap *= 2;
        arr->ptrs = reallocate(arr->ptrs, arr->cap * sizeof(void*));
    }
    arr->ptrs[arr->cnt] = ptr;
    arr->cnt++;
}

// moves contents of ptr array onto arena and release ptr array
static void **mv_ptr_array_to_arena(struct Arena *arena, struct PtrArray *arr) 
{
    void **nodes = push_arena(arena, arr->cnt * sizeof(void*));
    for (i32 i = 0; i < arr->cnt; i++)
        nodes[i] = arr->ptrs[i];
    release_ptr_array(arr);
    return nodes;
}

struct IdentArray {
    u32 cap;
    u32 cnt;
    struct IdentNode *idents;
};

static void init_ident_array(struct IdentArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 8;
    arr->idents = allocate(arr->cap * sizeof(struct IdentNode));
}

static void release_ident_array(struct IdentArray *arr) 
{
    arr->cnt = 0;
    arr->cap = 0;
    release(arr->idents);
    arr->idents = NULL;
}

static void push_ident_array(struct IdentArray *arr, struct IdentNode ident) 
{
    if (arr->cnt == arr->cap) {
        arr->cap *= 2;
        arr->idents = reallocate(arr->idents, arr->cap * sizeof(struct IdentNode));
    }
    arr->idents[arr->cnt] = ident;
    arr->cnt++;
}

// moves contents of span array onto arena and release span array
static struct IdentNode *mv_ident_array_to_arena(struct Arena *arena, struct IdentArray *arr) 
{
    struct IdentNode *idents = push_arena(arena, arr->cnt * sizeof(struct IdentNode));
    for (i32 i = 0; i < arr->cnt; i++)
        idents[i] = arr->idents[i];
    release_ident_array(arr);
    return idents;
}

static struct AtomNode *mk_atom(struct Arena *arena, struct Token token) 
{
    struct AtomNode *node = push_arena(arena, sizeof(struct AtomNode));
    node->base.tag = NODE_ATOM;
    node->base.span = token.span;
    node->atom_tag = token.tag;
    return node;
}

static struct ListNode *mk_list(struct Arena *arena, struct Span span, struct Node **items, u32 cnt)
{
    struct ListNode *node = push_arena(arena, sizeof(struct ListNode));
    node->base.span = span;
    node->base.tag = NODE_LIST;
    node->items = items;
    node->cnt = cnt;
    return node;
}

static struct IdentNode *mk_ident(struct Arena *arena, struct Span span) 
{
    struct IdentNode *node = push_arena(arena, sizeof(struct IdentNode));
    node->base.tag = NODE_IDENT;
    node->base.span = span;
    node->id = -1;
    return node;
}

static struct UnaryNode *mk_unary(struct Arena *arena, struct Token token, struct Node *rhs) 
{
    struct UnaryNode *node = push_arena(arena, sizeof(struct UnaryNode));
    node->base.tag = NODE_UNARY;
    node->base.span = token.span;
    node->op_tag = token.tag;
    node->rhs = rhs;
    return node;
}

static struct BinaryNode *mk_binary(
    struct Arena *arena, 
    struct Span span, 
    enum TokenTag tag, 
    struct Node *lhs, 
    struct Node *rhs
) {
    struct BinaryNode *node = push_arena(arena, sizeof(struct BinaryNode));
    node->base.tag = NODE_BINARY;
    node->base.span = span;
    node->op_tag = tag;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static struct PropNode *mk_prop(struct Arena *arena, struct Span span, struct Node *lhs, struct Span prop)
{
    struct PropNode *node = push_arena(arena, sizeof(struct PropNode));
    node->base.span = span;
    node->base.tag = NODE_PROP;
    node->lhs = lhs;
    node->prop = prop;
    return node;
}

static struct FnCallNode *mk_fn_call(struct Arena *arena, struct Span span, struct Node *lhs, struct Node **args, u32 arity) 
{
    struct FnCallNode *node = push_arena(arena, sizeof(struct FnCallNode));
    node->base.tag = NODE_FN_CALL;
    node->base.span = span;
    node->lhs = lhs;
    node->args = args;
    node->arity = arity;
    return node;
}

static struct VarDeclNode *mk_var_decl(struct Arena *arena, struct Span span, struct Node *init)
{
    struct VarDeclNode *node = push_arena(arena, sizeof(struct VarDeclNode));
    node->base.tag = NODE_VAR_DECL;
    node->base.span = span;
    node->id = -1;
    node->init = init;
    return node;
}

static struct FnDeclNode *mk_fn_decl(
    struct Arena *arena, 
    struct Span span, 
    struct IdentNode *params, 
    u32 arity, 
    struct BlockNode *body
) {
    struct FnDeclNode *node = push_arena(arena, sizeof(struct FnDeclNode));
    node->base.tag = NODE_FN_DECL;
    node->base.span = span;
    node->params = params;
    node->arity = arity;
    node->body = body;
    node->id = -1;
    node->stack_capture_cnt = 0;
    node->parent_capture_cnt = 0;
    node->parent = NULL;
    return node;
}

static struct ExprStmtNode *mk_expr_stmt(struct Arena *arena, struct Span span, struct Node *expr)
{
    struct ExprStmtNode *node = push_arena(arena, sizeof(struct ExprStmtNode));
    node->base.tag = NODE_EXPR_STMT;
    node->base.span = span;
    node->expr = expr;
    return node;
}

static struct IfNode *mk_if(
    struct Arena *arena, 
    struct Span span, 
    struct Node *cond, 
    struct BlockNode *thn, 
    struct BlockNode *els
) {
    struct IfNode *node = push_arena(arena, sizeof(struct IfNode));
    node->base.tag = NODE_IF;
    node->base.span = span;
    node->cond = cond;
    node->thn = thn;
    node->els = els;
    return node;
}

static struct BlockNode *mk_block(struct Arena *arena, struct Span span, struct Node **stmts, u32 cnt) 
{
    struct BlockNode *node = push_arena(arena, sizeof(struct BlockNode));
    node->base.tag = NODE_BLOCK;
    node->base.span = span;
    node->stmts = stmts;
    node->cnt = cnt;
    return node;
}

static struct Token at(struct Parser *parser) 
{
    return parser->at;
}

static struct Token prev(struct Parser *parser) 
{
    return parser->prev;
}

static void bump(struct Parser *parser)
{
    parser->prev = parser->at;
    parser->at = next_token(&parser->scanner);
}

static bool eat(struct Parser *parser, enum TokenTag tag) 
{
    if (at(parser).tag == tag) {
        bump(parser);
        return true;
    }
    return false;
}

static void emit_err(struct Parser *parser, struct Span span, const char *msg)
{
    if (!parser->panic)
        push_errlist(&parser->errlist, at(parser).span, msg);
    parser->panic = true;
}

static bool expect(struct Parser *parser, enum TokenTag tag, const char *msg) 
{
    if (eat(parser, tag))
        return true;
    emit_err(parser, at(parser).span, msg);
    return false;
}

static void advance_with_err(struct Parser *parser, const char *msg) 
{
    emit_err(parser, at(parser).span, msg);
    bump(parser);
}

// discard tokens until reach a starting token (e.g. var, if) or exit scope
static void recover_block(struct Parser *parser) 
{
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

struct PrecLvl {
    u32 old;
    u32 new;
};

static struct PrecLvl infix_prec(enum TokenTag tag) 
{
    switch (tag) {
    case TOKEN_EQ:
    case TOKEN_PLUS_EQ:
    case TOKEN_MINUS_EQ:
    case TOKEN_STAR_EQ:
    case TOKEN_SLASH_EQ:
    case TOKEN_SLASH_SLASH_EQ:
    case TOKEN_PERCENT_EQ:
        return (struct PrecLvl){.old = 1, .new = 2};
    case TOKEN_OR:
        return (struct PrecLvl){.old = 3, .new = 3};
    case TOKEN_AND:
        return (struct PrecLvl){.old = 5, .new = 5};
    case TOKEN_EQEQ:
    case TOKEN_NEQ:
        return (struct PrecLvl){.old = 7, .new = 8};
    case TOKEN_LT:
    case TOKEN_LEQ:
    case TOKEN_GT:
    case TOKEN_GEQ:
        return (struct PrecLvl){.old = 9, .new = 10};
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return (struct PrecLvl){.old = 11, .new = 12};
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_SLASH_SLASH:
    case TOKEN_PERCENT:
        return (struct PrecLvl){.old = 13, .new = 14};
    // we view `[` as binary operator lhs[rhs]
    // the old precedence is high because the operator binds tightly
    //      e.g. x * y[0] is parsed as x * (y[0])
    // but the expression within the [ ] should be parsed from the starting 
    // precedence level, similar to show ( ) resets the precedence level
    case TOKEN_L_SQUARE:
        return (struct PrecLvl){.old = 15, .new = 1};
    default:
        return (struct PrecLvl){.old = 0, .new = 0};
    }
}

// returns whether this token can start an expression
static bool expr_first(enum TokenTag tag)
{
    // TODO add strings
    return tag == TOKEN_TRUE 
        || tag == TOKEN_FALSE
        || tag == TOKEN_NUMBER
        || tag == TOKEN_STRING
        || tag == TOKEN_IDENTIFIER
        || tag == TOKEN_L_SQUARE
        || tag == TOKEN_L_PAREN
        || tag == TOKEN_MINUS
        || tag == TOKEN_NOT;
}

// e.g. given += return + and return -1 if cannot be desugared
static i32 desugar(enum TokenTag tag) {
    switch(tag) {
    case TOKEN_PLUS_EQ:        return TOKEN_PLUS;    
    case TOKEN_MINUS_EQ:       return TOKEN_MINUS;
    case TOKEN_STAR_EQ:        return TOKEN_STAR;
    case TOKEN_SLASH_EQ:       return TOKEN_SLASH;
    case TOKEN_SLASH_SLASH_EQ: return TOKEN_SLASH_SLASH;
    case TOKEN_PERCENT_EQ:     return TOKEN_PERCENT;
    default:                   return -1;
    }
}

static struct Node *parse_expr(struct Parser *parser, u32 prec_lvl);

// precondition: `[` or `(` token consumed
// parses arguments and fills the ptr array provided
static void parse_arg_list(struct Parser *parser, struct PtrArray *args_tmp, enum TokenTag tag_right)
{
    while(at(parser).tag != tag_right && at(parser).tag != TOKEN_EOF) {
        // breaking early helps when the right token is missing
        if (!expr_first(at(parser).tag)) {
            emit_err(parser, at(parser).span, "expected expression");
            break;
        }
        struct Node *arg = parse_expr(parser, 1);
        push_ptr_array(args_tmp, arg);
        if (at(parser).tag != tag_right)
            expect(parser, TOKEN_COMMA, "expected `,`");            
    }
}

static struct Node *parse_expr(struct Parser *parser, u32 prec_lvl) 
{
    struct Token token = at(parser);
    if (expr_first(token.tag)) // we could bump in each arm of the switch but this is simpler
        bump(parser);
    struct Node *lhs = NULL;
    // TODO add strings
    switch (token.tag) {
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NUMBER:
    case TOKEN_STRING:
        lhs = (struct Node*)mk_atom(&parser->arena, token);
        break;
    case TOKEN_IDENTIFIER:
        lhs = (struct Node*)mk_ident(&parser->arena, token.span);
        break;
    case TOKEN_L_SQUARE:
        struct PtrArray items_tmp;
        init_ptr_array(&items_tmp);
        parse_arg_list(parser, &items_tmp, TOKEN_R_SQUARE);
        expect(parser, TOKEN_R_SQUARE, "expected `]`");
        u32 cnt = items_tmp.cnt;
        struct Node **items = (struct Node**)mv_ptr_array_to_arena(&parser->arena, &items_tmp);
        lhs = (struct Node*)mk_list(&parser->arena, token.span, items, cnt);
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
        emit_err(parser, token.span, "expected expression");
        return NULL;
    }
    while(true) {
        if (eat(parser, TOKEN_DOT)) {
            struct Span span = prev(parser).span;
            expect(parser, TOKEN_IDENTIFIER, "expected identifier");
            lhs = (struct Node*)mk_prop(&parser->arena, span, lhs, prev(parser).span);
            continue;
        }
        // parse fn_call
        if (eat(parser, TOKEN_L_PAREN)) {
            struct Span fn_call_span = prev(parser).span;
            struct PtrArray args_tmp;
            init_ptr_array(&args_tmp);
            parse_arg_list(parser, &args_tmp, TOKEN_R_PAREN);
            expect(parser, TOKEN_R_PAREN, "expected `)`");
            u32 cnt = args_tmp.cnt;
            struct Node **args = (struct Node **)mv_ptr_array_to_arena(&parser->arena, &args_tmp);
            lhs = (struct Node*)mk_fn_call(&parser->arena, fn_call_span, lhs, args, cnt);
            continue;
        }
        token = at(parser);
        struct PrecLvl prec = infix_prec(token.tag);
        u32 old_lvl = prec.old;
        u32 new_lvl = prec.new;
        if (old_lvl < prec_lvl) {
            break;
        }
        bump(parser);
        struct Node *rhs = parse_expr(parser, new_lvl);
        // since we view `[` as a binary operator we have to consume `]` after parsing idx expr
        if (token.tag == TOKEN_L_SQUARE)
            expect(parser, TOKEN_R_SQUARE, "expected `]`");
        
        enum TokenTag tag = token.tag;
        // desugar +=, -=, *=, /=, //=, and %=
        enum TokenTag desugared = desugar(tag);
        if (desugared != -1) {
            rhs = (struct Node*) mk_binary(&parser->arena, token.span, desugared, lhs, rhs);
            tag = TOKEN_EQ;
        }
        lhs = (struct Node*) mk_binary(&parser->arena, token.span, tag, lhs, rhs);
    }
    return lhs;
}

static struct BlockNode *parse_block(struct Parser *parser);

// precondition: `if` token consumed
static struct IfNode *parse_if(struct Parser *parser) 
{
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
static struct VarDeclNode *parse_var_decl(struct Parser *parser) 
{
    // in a case such as 
    //      var x 4;
    // we assume the user forgot to add an `=` after `x`
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    if (eat(parser, TOKEN_SEMI))
        return mk_var_decl(&parser->arena, span, NULL);
    expect(parser, TOKEN_EQ, "expected `=`");
    struct Node *init = parse_expr(parser, 1);
    expect(parser, TOKEN_SEMI, "expected `;`");
    return mk_var_decl(&parser->arena, span, init);
}

// precondition: `fn` token consumed
static struct FnDeclNode *parse_fn_decl(struct Parser *parser) 
{
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    expect(parser, TOKEN_L_PAREN, "expected `(`");  
    struct IdentArray tmp_params;
    init_ident_array(&tmp_params);
    while(at(parser).tag != TOKEN_R_PAREN && at(parser).tag != TOKEN_EOF) {
        if (eat(parser, TOKEN_IDENTIFIER)) {
            struct IdentNode ident = {
                .base.span = prev(parser).span,
                .id = -1,
            };
            push_ident_array(&tmp_params, ident);
            if (at(parser).tag != TOKEN_R_PAREN)
                expect(parser, TOKEN_COMMA, "expected `,`");
        } else {
            advance_with_err(parser, "expected identifier");
            if (at(parser).tag == TOKEN_FN || at(parser).tag == TOKEN_IMPORT) {
                parser->panic = false;
                release_ident_array(&tmp_params);
                return NULL;
            }
        }
    }
    expect(parser, TOKEN_R_PAREN, "expected `)`");
    u32 arity = tmp_params.cnt;
    struct IdentNode *param_spans = mv_ident_array_to_arena(&parser->arena, &tmp_params);
    struct BlockNode *body = parse_block(parser);
    return mk_fn_decl(&parser->arena, span, param_spans, arity,  body);
}

// precondition: `return` token consumed
static struct ReturnNode *parse_return(struct Parser *parser) 
{
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
static struct PrintNode *parse_print(struct Parser *parser) 
{
    struct Span span = prev(parser).span;
    struct Node *expr = parse_expr(parser, 1);
    struct PrintNode *node = push_arena(&parser->arena, sizeof(struct PrintNode));
    node->base.tag = NODE_PRINT;
    node->base.span = span;
    node->expr = expr;
    expect(parser, TOKEN_SEMI, "expected `;`");
    return node;
}

static struct BlockNode *parse_block(struct Parser *parser) 
{
    struct Span span = at(parser).span;
    if (!expect(parser, TOKEN_L_BRACE, "expected `{`"))
        return NULL;
    struct PtrArray tmp;
    init_ptr_array(&tmp);
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
        } else if (eat(parser, TOKEN_FN)) {
            node = (struct Node*)parse_fn_decl(parser);
        } else if (eat(parser, TOKEN_PRINT)) {
            node = (struct Node*)parse_print(parser);
        } else if (expr_first(at(parser).tag)) {
            node = parse_expr(parser, 1);
            expect(parser, TOKEN_SEMI, "expected `;`"); 
            node = (struct Node*)mk_expr_stmt(&parser->arena, prev(parser).span, node); 
        } else {
            advance_with_err(parser, "expected statement");
        }
        push_ptr_array(&tmp, node);
        if (parser->panic)
            recover_block(parser);
    }
    expect(parser, TOKEN_R_BRACE, "expected `}`");
    u32 cnt = tmp.cnt;
    struct Node **stmts = (struct Node**)mv_ptr_array_to_arena(&parser->arena, &tmp);
    return mk_block(&parser->arena, span, stmts, cnt);
}

// precondition: `import` token consumed
static struct ImportNode *parse_import(struct Parser *parser)
{
    struct Span path = at(parser).span;
    expect(parser, TOKEN_STRING, "expected string");
    expect(parser, TOKEN_AS, "expected `as`");
    struct Span span = at(parser).span;
    expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    struct ImportNode *node = push_arena(&parser->arena, sizeof(struct ImportNode));
    node->base.span = span;
    node->base.tag = NODE_IMPORT;
    node->path = path;
    return node;
}

// for now, a file is implicitly a fn except 
// the body can only contain fn decls and mutual recursion is allowed
static struct FnDeclNode *parse_file(struct Parser *parser) 
{
    struct PtrArray tmp;
    init_ptr_array(&tmp);
    while (at(parser).tag != TOKEN_EOF) {
        struct Node *node = NULL;
        if (eat(parser, TOKEN_FN))
            node = (struct Node*)parse_fn_decl(parser);
        else
            advance_with_err(parser, "expected declaration");
        push_ptr_array(&tmp, node);
    }
    u32 cnt = tmp.cnt;
    struct Node **stmts = (struct Node**)mv_ptr_array_to_arena(&parser->arena, &tmp);
    struct Span span = {
        .start = "script", 
        .len = 7, 
        .line = 1,
    };
    struct BlockNode *body = mk_block(&parser->arena, span, stmts, cnt);
    return mk_fn_decl(&parser->arena, span, NULL, 0, body);
}

// TODO maybe disallow trailing comma in function declarations and calls
struct FnDeclNode *parse(struct Parser *parser, const char *source) 
{
    // TODO I should probably be clearing the errorlist each time
    init_scanner(&parser->scanner, source);
    parser->at = next_token(&parser->scanner);
    parser->panic = false;
    return parse_file(parser);
}