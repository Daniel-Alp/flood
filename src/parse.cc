#include "parse.h"
#include "arena.h"
#include "dynarr.h"
#include "scan.h"

// returns whether token can start an expression
static bool expr_first(const TokenTag tag)
{
    return tag == TOKEN_NULL || tag == TOKEN_TRUE || tag == TOKEN_FALSE || tag == TOKEN_NUMBER || tag == TOKEN_STRING ||
           tag == TOKEN_IDENTIFIER || tag == TOKEN_L_SQUARE || tag == TOKEN_L_PAREN || tag == TOKEN_MINUS ||
           tag == TOKEN_NOT;
}

struct PrecLvl {
    const i32 curr;
    const i32 next;
};

// returns precedence of infix operator
static struct PrecLvl infix_prec(TokenTag tag)
{
    switch (tag) {
    case TOKEN_EQ:
    case TOKEN_PLUS_EQ:
    case TOKEN_MINUS_EQ:
    case TOKEN_STAR_EQ:
    case TOKEN_SLASH_EQ:
    case TOKEN_SLASH_SLASH_EQ:
    case TOKEN_PERCENT_EQ: return {.curr = 1, .next = 2};
    case TOKEN_OR: return {.curr = 3, .next = 3};
    case TOKEN_AND: return {.curr = 5, .next = 5};
    case TOKEN_EQEQ:
    case TOKEN_NEQ: return {.curr = 7, .next = 8};
    case TOKEN_LT:
    case TOKEN_LEQ:
    case TOKEN_GT:
    case TOKEN_GEQ: return {.curr = 9, .next = 10};
    case TOKEN_PLUS:
    case TOKEN_MINUS: return {.curr = 11, .next = 12};
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_SLASH_SLASH:
    case TOKEN_PERCENT: return {.curr = 13, .next = 14};
    // we view `[` as binary operator lhs[rhs]
    // the old precedence is high because the operator binds tightly
    //      e.g. x * y[0] is parsed as x * (y[0])
    // but the expression within the [ ] should be parsed from the starting
    // precedence level, similar to how ( ) resets the precedence level
    case TOKEN_L_SQUARE: return {.curr = 15, .next = 1};
    default: return {.curr = 0, .next = 0};
    }
}

// e.g. given += return + and return -1 if cannot be desugared
static i32 desugar(const TokenTag tag)
{
    switch (tag) {
    case TOKEN_PLUS_EQ: return TOKEN_PLUS;
    case TOKEN_MINUS_EQ: return TOKEN_MINUS;
    case TOKEN_STAR_EQ: return TOKEN_STAR;
    case TOKEN_SLASH_EQ: return TOKEN_SLASH;
    case TOKEN_SLASH_SLASH_EQ: return TOKEN_SLASH_SLASH;
    case TOKEN_PERCENT_EQ: return TOKEN_PERCENT;
    default: return -1;
    }
}

Arena &Parser::arena() const
{
    return arena_;
}

Token Parser::at() const
{
    return at_;
}

Token Parser::prev() const
{
    return prev_;
}

bool Parser::panic() const
{
    return panic_;
}

void Parser::set_panic(const bool panic)
{
    panic_ = panic;
}

void Parser::bump()
{
    prev_ = at_;
    at_ = scanner.next_token();
}

bool Parser::eat(TokenTag tag)
{
    if (at_.tag == tag) {
        bump();
        return true;
    }
    return false;
}

void Parser::emit_err(const char *msg)
{
    if (!panic_)
        errarr.push(ErrMsg{at_.span, msg});
    panic_ = true;
}

bool Parser::expect(TokenTag tag, const char *msg)
{
    if (eat(tag))
        return true;
    emit_err(msg);
    return false;
}

void Parser::advance_with_err(const char *msg)
{
    emit_err(msg);
    bump();
}

void Parser::recover_block()
{
    panic_ = false;
    i32 depth = 0;
    while (prev_.tag != TOKEN_SEMI && at_.tag != TOKEN_EOF) {
        switch (at_.tag) {
        case TOKEN_IF:
        case TOKEN_VAR:
        case TOKEN_FN:
        case TOKEN_CLASS: return;
        case TOKEN_L_BRACE: depth++; break;
        case TOKEN_R_BRACE: depth--;
        default: break;
        }
        if (depth == -1)
            return;
        bump();
    }
}

static Node *parse_expr(Parser &p, const i32 prec_lvl);
static BlockNode *parse_block(Parser &p);

// precondition: `[` or `(` token consumed
// parses arguments and fills the ptr array provided
static Dynarr<Node *> parse_arg_list(Parser &p, const TokenTag tag_right)
{
    Dynarr<Node *> nodearr;
    while (p.at().tag != tag_right && p.at().tag != TOKEN_EOF) {
        // breaking early helps when the right token is missing
        if (!expr_first(p.at().tag)) {
            p.emit_err("expected expression");
            break;
        }
        nodearr.push(parse_expr(p, 1));
        if (p.at().tag != tag_right)
            p.expect(TOKEN_COMMA, "expected `,`");
    }
    return nodearr;
}

static Node *parse_expr(Parser &p, const i32 prec_lvl)
{
    Token token = p.at();
    if (expr_first(token.tag)) // we could bump in each arm of the switch but this is simpler
        p.bump();
    Node *lhs = nullptr;
    switch (token.tag) {
    case TOKEN_NULL:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NUMBER:
    case TOKEN_STRING: lhs = alloc<AtomNode>(p.arena(), token.span, token.tag); break;
    case TOKEN_IDENTIFIER: lhs = alloc<IdentNode>(p.arena(), token.span); break;
    case TOKEN_L_SQUARE: {
        Dynarr<Node *> nodearr = parse_arg_list(p, TOKEN_R_SQUARE);
        p.expect(TOKEN_R_SQUARE, "expected `]`");
        const i32 cnt = nodearr.len();
        Node *const *const items = move_dynarr(p.arena(), move(nodearr));
        lhs = alloc<ListNode>(p.arena(), token.span, items, cnt);
        break;
    }
    case TOKEN_L_PAREN:
        lhs = parse_expr(p, 1);
        p.expect(TOKEN_R_PAREN, "expected `)`");
        break;
    case TOKEN_MINUS:
    case TOKEN_NOT: lhs = alloc<UnaryNode>(p.arena(), token.span, parse_expr(p, 15), token.tag); break;
    case TOKEN_ERR:
        // we already emitted an error for TOKEN_ERR tokens
        p.set_panic(true);
        return nullptr;
    default: p.emit_err("expected expression"); return nullptr;
    }
    while (true) {
        if (p.eat(TOKEN_DOT) || p.eat(TOKEN_COLON)) {
            const Token token = p.prev();
            p.expect(TOKEN_IDENTIFIER, "expected identifier");
            lhs = alloc<PropertyNode>(p.arena(), token.span, lhs, p.prev().span, p.prev().tag);
            continue;
        }
        // parse fn_call
        if (p.eat(TOKEN_L_PAREN)) {
            const Span fn_call_span = p.prev().span;
            Dynarr<Node *> nodearr = parse_arg_list(p, TOKEN_R_PAREN);
            p.expect(TOKEN_R_PAREN, "expected `)`");
            const i32 cnt = nodearr.len();
            Node *const *const args = move_dynarr(p.arena(), move(nodearr));
            lhs = alloc<CallNode>(p.arena(), fn_call_span, lhs, args, cnt);
            continue;
        }
        token = p.at();
        const PrecLvl prec = infix_prec(token.tag);
        if (prec.curr < prec_lvl) {
            break;
        }
        p.bump();
        Node *rhs = parse_expr(p, prec.next);
        // since we view `[` as a binary operator we have to consume `]` after parsing idx expr
        if (token.tag == TOKEN_L_SQUARE)
            p.expect(TOKEN_R_SQUARE, "expected `]`");

        enum TokenTag tag = token.tag;
        // desugar +=, -=, *=, /=, //=, and %=
        const i32 desugared = desugar(tag);
        if (desugared != -1) {
            rhs = alloc<BinaryNode>(p.arena(), token.span, lhs, rhs, TokenTag(desugared));
            tag = TOKEN_EQ;
        }
        lhs = alloc<BinaryNode>(p.arena(), token.span, lhs, rhs, tag);
    }
    return lhs;
}

// precondition: `if` token consumed
static IfNode *parse_if(Parser &p)
{
    const Span span = p.prev().span;
    p.expect(TOKEN_L_PAREN, "expected `(`");
    Node *const cond = parse_expr(p, 1);
    p.expect(TOKEN_R_PAREN, "expected `)`");
    BlockNode *const thn = parse_block(p);
    BlockNode *els = nullptr;
    if (p.eat(TOKEN_ELSE))
        els = parse_block(p);
    return alloc<IfNode>(p.arena(), span, cond, thn, els);
}

// precondition: `var` token consumed
static VarDeclNode *parse_var_decl(Parser &p)
{
    // in a case such as
    //      var x 4;
    // we assume the user forgot to add an `=` after `x`
    const Span span = p.at().span;
    p.expect(TOKEN_IDENTIFIER, "expected identifier");
    if (p.eat(TOKEN_SEMI))
        return alloc<VarDeclNode>(p.arena(), span, nullptr);
    p.expect(TOKEN_EQ, "expected `=`");
    Node *const init = parse_expr(p, 1);
    p.expect(TOKEN_SEMI, "expected `;`");
    return alloc<VarDeclNode>(p.arena(), span, init);
}

// precondition: `fn` token consumed
static FnDeclNode *parse_fn_decl(Parser &p, const bool is_method)
{
    const Span span = p.at().span;
    p.expect(TOKEN_IDENTIFIER, "expected identifier");
    p.expect(TOKEN_L_PAREN, "expected `(`");
    Dynarr<IdentNode> paramarr;
    while (p.at().tag != TOKEN_R_PAREN && p.at().tag != TOKEN_EOF) {
        if (p.eat(TOKEN_IDENTIFIER)) {
            paramarr.push(IdentNode(p.prev().span));
            if (p.at().tag != TOKEN_R_PAREN)
                p.expect(TOKEN_COMMA, "expected `,`");
        } else if (p.at().tag == TOKEN_FN || p.at().tag == TOKEN_CLASS || p.at().tag == TOKEN_L_BRACE ||
                   p.at().tag == TOKEN_IMPORT) {
            // breaking early helps when there is no matching `)`
            break;
        } else {
            p.advance_with_err("expected identifier");
        }
    }
    if (is_method) {
        // TODO we should never need the line of `self` I believe
        paramarr.push(IdentNode(Span{.start = "self", .len = 4, .line = 0}));
    }
    p.expect(TOKEN_R_PAREN, "expected `)`");
    const i32 arity = paramarr.len();
    IdentNode *const params = move_dynarr(p.arena(), move(paramarr));
    BlockNode *const body = parse_block(p);
    return alloc<FnDeclNode>(p.arena(), span, body, params, arity);
}

// precondition: `return` token consumed
ReturnNode *parse_return(Parser &p)
{
    const Span span = p.prev().span;
    if (p.eat(TOKEN_SEMI))
        return alloc<ReturnNode>(p.arena(), span, nullptr);
    ReturnNode *node = alloc<ReturnNode>(p.arena(), span, parse_expr(p, 1));
    p.expect(TOKEN_SEMI, "expected `;`");
    return node;
}

// TEMP remove when we add functions
// precondition: `print` token consumed
PrintNode *parse_print(Parser &p)
{
    const Span span = p.prev().span;
    PrintNode *node = alloc<PrintNode>(p.arena(), span, parse_expr(p, 1));
    p.expect(TOKEN_SEMI, "expected `;`");
    return node;
}

BlockNode *parse_block(Parser &p)
{
    const Span span = p.at().span;
    if (!p.expect(TOKEN_L_BRACE, "expected `{`"))
        return nullptr;
    Dynarr<Node *> nodearr;
    while (p.at().tag != TOKEN_R_BRACE && p.at().tag != TOKEN_EOF) {
        Node *node = nullptr;
        if (p.at().tag == TOKEN_L_BRACE) {
            node = parse_block(p);
        } else if (p.eat(TOKEN_IF)) {
            node = parse_if(p);
        } else if (p.eat(TOKEN_VAR)) {
            node = parse_var_decl(p);
        } else if (p.eat(TOKEN_RETURN)) {
            node = parse_return(p);
        } else if (p.eat(TOKEN_FN)) {
            node = parse_fn_decl(p, false);
        } else if (p.eat(TOKEN_PRINT)) {
            node = parse_print(p);
        } else if (expr_first(p.at().tag)) {
            node = parse_expr(p, 1);
            p.expect(TOKEN_SEMI, "expected `;`");
            node = alloc<ExprStmtNode>(p.arena(), p.prev().span, node);
        } else {
            p.advance_with_err("expected statement");
        }
        nodearr.push(node);
        if (p.panic())
            p.recover_block();
    }
    p.expect(TOKEN_R_BRACE, "expected `}`");
    const i32 cnt = nodearr.len();
    Node *const *const stmts = move_dynarr(p.arena(), move(nodearr));
    return alloc<BlockNode>(p.arena(), span, stmts, cnt);
}

// for now, a file is implicitly a fn except
// the body can only contain fn and class decls and mutual recursion is allowed
ModuleNode &parse_file(Parser &p)
{
    Dynarr<Node *> nodearr;
    while (p.at().tag != TOKEN_EOF) {
        if (p.eat(TOKEN_FN)) {
            p.set_panic(false);
            nodearr.push(parse_fn_decl(p, false));
        } else {
            p.advance_with_err("expected declaration");
        }
    }
    const i32 cnt = nodearr.len();
    Node *const *const decls = move_dynarr(p.arena(), move(nodearr));
    const Span span = {.start = "module", .len = 7, .line = 1};
    return *alloc<ModuleNode>(p.arena(), span, decls, cnt);
}

// TODO consider parser owning Arena
ModuleNode &Parser::parse(const char *source, Arena &arena, Dynarr<ErrMsg> &errarr)
{
    Parser p(source, arena, errarr);
    return parse_file(p);
}
