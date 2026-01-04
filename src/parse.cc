#include "parse.h"
#include "arena.h"
#include "dynarr.h"
#include "scan.h"

// returns whether token can start an expression
static bool expr_first(const TokenTag tag)
{
    return tag == TOKEN_NULL
        || tag == TOKEN_TRUE 
        || tag == TOKEN_FALSE
        || tag == TOKEN_NUMBER
        || tag == TOKEN_STRING
        || tag == TOKEN_IDENTIFIER
        || tag == TOKEN_L_SQUARE
        || tag == TOKEN_L_PAREN
        || tag == TOKEN_MINUS
        || tag == TOKEN_NOT;
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
    case TOKEN_PERCENT_EQ:
        return {.curr = 1, .next = 2};
    case TOKEN_OR:
        return {.curr = 3, .next = 3};
    case TOKEN_AND:
        return {.curr = 5, .next = 5};
    case TOKEN_EQEQ:
    case TOKEN_NEQ:
        return {.curr = 7, .next = 8};
    case TOKEN_LT:
    case TOKEN_LEQ:
    case TOKEN_GT:
    case TOKEN_GEQ:
        return {.curr = 9, .next = 10};
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return {.curr = 11, .next = 12};
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_SLASH_SLASH:
    case TOKEN_PERCENT:
        return {.curr = 13, .next = 14};
    // we view `[` as binary operator lhs[rhs]
    // the old precedence is high because the operator binds tightly
    //      e.g. x * y[0] is parsed as x * (y[0])
    // but the expression within the [ ] should be parsed from the starting 
    // precedence level, similar to how ( ) resets the precedence level
    case TOKEN_L_SQUARE:
        return {.curr = 15, .next = 1};
    default:
        return {.curr = 0, .next = 0};
    }
}

// e.g. given += return + and return -1 if cannot be desugared
static i32 desugar(const TokenTag tag) {
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

void Parser::bump()
{
    prev = at;
    at = scanner.next_token();
}

bool Parser::eat(TokenTag tag)
{
    if (at.tag == tag) {
         bump();
         return true;
     }
     return false;
}

void Parser::emit_err(const char *msg)
{
    if (!panic)
        errarr.push(ErrMsg{at.span, msg});
    panic = true;
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
    panic = false;
    i32 depth = 0;
    while (prev.tag != TOKEN_SEMI && at.tag != TOKEN_EOF) {
        switch(at.tag) {
        case TOKEN_IF:
        case TOKEN_VAR:
        case TOKEN_FN:
        case TOKEN_CLASS:
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
        bump();
    }
}   

// precondition: `[` or `(` token consumed
// parses arguments and fills the ptr array provided
Dynarr<Node*> Parser::parse_arg_list(const TokenTag tag_right)
{
    Dynarr<Node*> nodearr;
    while(at.tag != tag_right && at.tag != TOKEN_EOF) {
        // breaking early helps when the right token is missing
        if (!expr_first(at.tag)) {
            emit_err("expected expression");
            break;
        }
        nodearr.push(parse_expr(1));
        if (at.tag != tag_right)
            expect(TOKEN_COMMA, "expected `,`");            
    }
    return nodearr;
}

Node *Parser::parse_expr(const i32 prec_lvl) 
{
    Token token = at;
    if (expr_first(token.tag)) // we could bump in each arm of the switch but this is simpler
        bump();
    Node *lhs = nullptr;
    switch (token.tag) {
    case TOKEN_NULL:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NUMBER:
    case TOKEN_STRING:
        lhs = alloc<AtomNode>(arena, token.span, token.tag);
        break;
    case TOKEN_IDENTIFIER:
        lhs = alloc<IdentNode>(arena, token.span);
        break;
    case TOKEN_L_SQUARE: {
        Dynarr<Node*> nodearr = parse_arg_list(TOKEN_R_SQUARE);
        expect(TOKEN_R_SQUARE, "expected `]`");
        const i32 cnt = nodearr.size();
        Node *const *const items = move_dynarr(arena, static_cast<Dynarr<Node*>&&>(nodearr));
        lhs = alloc<ListNode>(arena, token.span, items, cnt);
        break;
    }
    case TOKEN_L_PAREN:
        lhs = parse_expr(1);
        expect(TOKEN_R_PAREN, "expected `)`");
        break;
    case TOKEN_MINUS:
    case TOKEN_NOT:
        lhs = alloc<UnaryNode>(arena, token.span, parse_expr(15), token.tag);
        break;
    case TOKEN_ERR:
        // we already emitted an error for TOKEN_ERR tokens
        panic = true;
        return nullptr;
    default:
        emit_err("expected expression");
        return nullptr;
    }
    while(true) {
        if (eat(TOKEN_DOT) || eat( TOKEN_COLON)) {
            const Token token = prev;
            expect(TOKEN_IDENTIFIER, "expected identifier");
            lhs = alloc<PropertyNode>(arena, token.span, lhs, prev.span, prev.tag);
            continue;
        }
        // parse fn_call
        if (eat(TOKEN_L_PAREN)) {
            const Span fn_call_span = prev.span;
            Dynarr<Node*> nodearr = parse_arg_list(TOKEN_R_PAREN);
            expect(TOKEN_R_PAREN, "expected `)`");
            const i32 cnt = nodearr.size();
            Node *const *const args = move_dynarr(arena, static_cast<Dynarr<Node*>&&>(nodearr));
            lhs = alloc<CallNode>(arena, fn_call_span, lhs, args, cnt);
            continue;
        }
        token = at;
        const PrecLvl prec = infix_prec(token.tag);
        if (prec.curr < prec_lvl) {
            break;
        }
        bump();
        Node *rhs = parse_expr(prec.next);
        // since we view `[` as a binary operator we have to consume `]` after parsing idx expr
        if (token.tag == TOKEN_L_SQUARE)
            expect(TOKEN_R_SQUARE, "expected `]`");
        
        enum TokenTag tag = token.tag;
        // desugar +=, -=, *=, /=, //=, and %=
        const i32 desugared = desugar(tag);
        if (desugared != -1) {
            rhs = alloc<BinaryNode>(arena, token.span, lhs, rhs, TokenTag(desugared));
            tag = TOKEN_EQ;
        }
        lhs = alloc<BinaryNode>(arena, token.span, lhs, rhs, tag);
    }
    return lhs;
}

// precondition: `if` token consumed
IfNode *Parser::parse_if() 
{
    const Span span = prev.span;
    expect(TOKEN_L_PAREN, "expected `(`");
    Node *const cond = parse_expr(1);
    expect(TOKEN_R_PAREN, "expected `)`");
    BlockNode *const thn = parse_block();
    BlockNode *els = nullptr;
    if (eat(TOKEN_ELSE))
        els = parse_block();
    return alloc<IfNode>(arena, span, cond, thn, els);
}

// precondition: `var` token consumed
VarDeclNode *Parser::parse_var_decl() 
{
    // in a case such as 
    //      var x 4;
    // we assume the user forgot to add an `=` after `x`
    const Span span = at.span;
    expect(TOKEN_IDENTIFIER, "expected identifier");
    if (eat(TOKEN_SEMI))
        return alloc<VarDeclNode>(arena, span, nullptr);
    expect(TOKEN_EQ, "expected `=`");
    Node *const init = parse_expr(1);
    expect(TOKEN_SEMI, "expected `;`");
    return alloc<VarDeclNode>(arena, span, init);    
}

// precondition: `fn` token consumed
FnDeclNode *Parser::parse_fn_decl(const bool is_method) 
{
    const Span span = at.span;
    expect(TOKEN_IDENTIFIER, "expected identifier");
    expect(TOKEN_L_PAREN, "expected `(`");  
    Dynarr<IdentNode> paramarr;
    while(at.tag != TOKEN_R_PAREN && at.tag != TOKEN_EOF) {
        if (eat(TOKEN_IDENTIFIER)) {
            paramarr.push(IdentNode(prev.span));
            if (at.tag != TOKEN_R_PAREN)
                expect(TOKEN_COMMA, "expected `,`");
        } else if (at.tag == TOKEN_FN || at.tag == TOKEN_CLASS || at.tag == TOKEN_L_BRACE || at.tag == TOKEN_IMPORT) {
            // breaking early helps when there is no matching `)`
            break;            
        } else {
            advance_with_err("expected identifier");
        }
    }
    if (is_method) {
        // TODO we should never need the line of `self` I believe
        paramarr.push(IdentNode(Span{.start = "self", .len = 4, .line = 0}));
    }
    expect(TOKEN_R_PAREN, "expected `)`");
    const i32 arity = paramarr.size();
    IdentNode *const params = move_dynarr(arena, static_cast<Dynarr<IdentNode>&&>(paramarr));
    BlockNode *const body = parse_block();
    return alloc<FnDeclNode>(arena, span, body, params, arity);    
}

ClassDeclNode *Parser::parse_class_decl()
{
    const Span span = at.span;
    expect(TOKEN_IDENTIFIER, "expected identifier");
    expect(TOKEN_L_BRACE, "expected `{`");

    Dynarr<FnDeclNode*> nodearr;
    while (at.tag != TOKEN_R_BRACE && at.tag != TOKEN_EOF) {
        if (eat(TOKEN_FN)) {
            panic = false;
            nodearr.push(parse_fn_decl(true));
        } else {
            advance_with_err("expected method declaration");
        }    
    }
    expect(TOKEN_R_BRACE, "expected `}`");
    const i32 cnt = nodearr.size();
    FnDeclNode *const *const methods = move_dynarr(arena, static_cast<Dynarr<FnDeclNode*>&&>(nodearr));
    return alloc<ClassDeclNode>(arena, span, methods, cnt);
}

// precondition: `return` token consumed
ReturnNode *Parser::parse_return() 
{
    const Span span = prev.span;
    if (eat(TOKEN_SEMI))
        return alloc<ReturnNode>(arena, span, nullptr);
    ReturnNode *node = alloc<ReturnNode>(arena, span, parse_expr(1));
    expect(TOKEN_SEMI, "expected `;`");
    return node;
}

// TEMP remove when we add functions
// precondition: `print` token consumed
PrintNode *Parser::parse_print() 
{
    const Span span = prev.span;
    PrintNode *node = alloc<PrintNode>(arena, span, parse_expr(1));
    expect(TOKEN_SEMI, "expected `;`");
    return node;
}

BlockNode *Parser::parse_block() 
{
    const Span span = at.span;
    if (!expect(TOKEN_L_BRACE, "expected `{`"))
        return nullptr;
    Dynarr<Node*> nodearr;
    while(at.tag != TOKEN_R_BRACE && at.tag != TOKEN_EOF) {
        Node *node = nullptr;
        if (at.tag == TOKEN_L_BRACE) {
            node = parse_block();
        } else if (eat(TOKEN_IF)) {
            node = parse_if();
        } else if (eat(TOKEN_VAR)) {
            node = parse_var_decl();
        } else if (eat(TOKEN_RETURN)) {
            node = parse_return();  
        } else if (eat(TOKEN_FN)) {
            node = parse_fn_decl(false);
        } else if (eat(TOKEN_CLASS)) {
            node = parse_class_decl();
        } else if (eat(TOKEN_PRINT)) {
            node = parse_print();
        } else if (expr_first(at.tag)) {
            node = parse_expr(1);
            expect(TOKEN_SEMI, "expected `;`"); 
            node = alloc<ExprStmtNode>(arena, prev.span, node);
        } else {
            advance_with_err("expected statement");
        }
        nodearr.push(node);
        if (panic)
            recover_block();
    }
    expect(TOKEN_R_BRACE, "expected `}`");
    const i32 cnt = nodearr.size();
    Node *const *const stmts = move_dynarr(arena, static_cast<Dynarr<Node*>&&>(nodearr));
    return alloc<BlockNode>(arena, span, stmts, cnt);    
}

// for now, a file is implicitly a fn except 
// the body can only contain fn and class decls and mutual recursion is allowed
FnDeclNode *Parser::parse_file() 
{
    Dynarr<Node*> nodearr;
    while (at.tag != TOKEN_EOF) {
        if (eat(TOKEN_FN)) {
            panic = false;
            nodearr.push(parse_fn_decl(false));
        } else if (eat(TOKEN_CLASS)) {
            panic = false;
            nodearr.push(parse_class_decl());
        } else {
            advance_with_err("expected declaration");
        }
    }
    const i32 cnt = nodearr.size();
    Node *const *const stmts = move_dynarr(arena, static_cast<Dynarr<Node*>&&>(nodearr));
    const Span span = {.start = "script", .len = 7, .line = 1};
    return alloc<FnDeclNode>(arena, span, alloc<BlockNode>(arena, span, stmts, cnt), nullptr, 0);
}

Node *Parser::parse(const char *source, Arena &arena, Dynarr<ErrMsg> &errarr)
{
    return Parser(source, arena, errarr).parse_file();
}
