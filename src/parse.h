#pragma once
#include "../libflood/arena.h"
#include "ast.h"
#include "error.h"

class Parser {
    Arena &arena_;
    Dynarr<ErrMsg> &errarr;
    Scanner scanner;

    Token at_;
    Token prev_;
    bool panic_;
    Parser(const char *source, Arena &arena, Dynarr<ErrMsg> &errarr)
        : arena_(arena), errarr(errarr), scanner(source, errarr)
    {
        at_ = scanner.next_token();
        panic_ = false;
    }

public:
    Arena &arena() const;
    Token at() const;
    Token prev() const;
    bool panic() const;
    void set_panic(const bool panic);
    void bump();
    bool eat(TokenTag tag);
    void emit_err(const char *msg);
    bool expect(TokenTag tag, const char *msg);
    void advance_with_err(const char *msg);
    void recover_block();
    static ModuleNode &parse(const char *source, Arena &arena, Dynarr<ErrMsg> &errarr);
};
