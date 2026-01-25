#include "debug.h"
#include "ast.h"
#include <stdio.h>

const char *loc_tag_str(const LocTag tag)
{
    // clang-format off
    switch(tag) {
    case LOC_LOCAL:             return "local";
    case LOC_GLOBAL:            return "global";
    case LOC_STACK_HEAPVAL:     return "stack_heapval";
    case LOC_CAPTURED_HEAPVAL:  return "captured_heapval";
    }
    // clang-format on
    return nullptr;
}

struct AstPrinter final : public AstVisitor {
    i32 offset;
    const bool verbose;
    AstPrinter(const bool verbose) : offset(0), verbose(verbose) {}

    void visit_atom(AtomNode &node) override
    {
        printf("Atom %.*s", node.span.len, node.span.start);
    }

    void visit_list(ListNode &node) override
    {
        printf("List");
        for (i32 i = 0; i < node.cnt; i++)
            visit_expr(*node.items[i]);
    }

    void visit_ident(IdentNode &node) override
    {
        printf("Ident %.*s", node.span.len, node.span.start);
        if (verbose)
            printf(": %p", node.decl);
    }

    void visit_unary(UnaryNode &node) override
    {
        printf("Unary\n");
        printf("%*s", offset, "");
        printf("%.*s", node.span.len, node.span.start);
        visit_expr(*node.rhs);
    }

    void visit_binary(BinaryNode &node) override
    {
        printf("Binary\n");
        printf("%*s", offset, "");
        printf("%.*s", node.span.len, node.span.start);
        visit_expr(*node.lhs);
        visit_expr(*node.rhs);
    }

    void visit_selector(SelectorNode &node) override
    {
        printf("Selector");
        visit_expr(*node.lhs);
        printf("\n%*s", offset, "");
        printf("%.*s", node.sym.len, node.sym.start);
    }

    void visit_subscr(SubscrNode &node) override
    {
        printf("Subscr");
        visit_expr(*node.lhs);
        visit_expr(*node.rhs);
    }

    void visit_assign(AssignNode &node) override
    {
        printf("Assign\n");
        printf("%*s", offset, "");
        printf("%.*s", node.span.len, node.span.start);
        visit_expr(*node.lhs);
        visit_expr(*node.rhs);
    }

    void visit_call(CallNode &node) override
    {
        printf("Call");
        visit_expr(*node.lhs);
        for (i32 i = 0; i < node.arity; i++)
            visit_expr(*node.args[i]);
    }

    void visit_expr(Node &node) override
    {
        printf("\n%*s(", offset, "");
        offset += 2;
        AstVisitor::visit_expr(node);
        printf(")");
        offset -= 2;
    }

    void visit_var_decl(VarDeclNode &node) override
    {
        printf("Var %.*s", node.span.len, node.span.start);
        if (verbose)
            printf(": %p, %s, %d", &node, loc_tag_str(node.loc.tag), node.loc.idx);
        if (node.init)
            visit_expr(*node.init);
    }

    void visit_fn_decl(FnDeclNode &node) override
    {
        printf("Fn %.*s(", node.span.len, node.span.start);
        for (i32 i = 0; i < node.arity - 1; i++) {
            const VarDeclNode param = node.params[i];
            printf("%.*s, ", param.span.len, param.span.start);
        }
        if (node.arity > 0) {
            const VarDeclNode param = node.params[node.arity - 1];
            printf("%.*s", param.span.len, param.span.start);
        }
        printf(")");
        if (verbose)
            printf(": %p, %s, %d", &node, loc_tag_str(node.loc.tag), node.loc.idx);
        if (verbose && node.capture_cnt > 0) {
            for (i32 i = 0; i < node.capture_cnt - 1; i++) {
                const CaptureDecl *capt = node.captures[i];
                printf("\n%*s", offset, "");
                printf("(Capture %.*s: %p, %s, %d)", capt->span.len, capt->span.start, 
                    capt, loc_tag_str(capt->loc.tag), capt->loc.idx);
            }
            const CaptureDecl *capt = node.captures[node.capture_cnt - 1];
            printf("\n%*s", offset, "");
            printf("(Capture %.*s: %p, %s, %d)", capt->span.len, capt->span.start, 
                capt, loc_tag_str(capt->loc.tag), capt->loc.idx);
        }
        visit_stmt(*node.body);
    }

    void visit_class_decl(ClassDeclNode &node) override
    {
        printf("Class %.*s", node.span.len, node.span.start);
        if (verbose)
            printf(": %p, %s, %d", &node, loc_tag_str(node.loc.tag), node.loc.idx);
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.methods[i]);
    }

    void visit_expr_stmt(ExprStmtNode &node) override
    {
        printf("ExprStmt");
        visit_expr(*node.expr);
    }

    void visit_return(ReturnNode &node) override
    {
        printf("Return");
        if (node.expr)
            visit_expr(*node.expr);
    }

    void visit_block(BlockNode &node) override
    {
        printf("Block");
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.stmts[i]);
    }

    void visit_if(IfNode &node) override
    {
        printf("If");
        visit_expr(*node.cond);
        visit_stmt(*node.thn);
        if (node.els)
            visit_stmt(*node.els);
    }

    void visit_print(PrintNode &node) override
    {
        printf("Print");
        visit_expr(*node.expr);
    }

    void visit_stmt(Node &node) override
    {
        printf("\n%*s(", offset, "");
        offset += 2;
        AstVisitor::visit_stmt(node);
        printf(")");
        offset -= 2;
    }

    // TODO make visit a method in AstVisitor?
    void visit(ModuleNode &node)
    {
        printf("(Module");
        offset += 2;
        for (i32 i = 0; i < node.cnt; i++)
            visit_stmt(*node.decls[i]);
        offset -= 2;
        printf(")\n");
    }
};

void print_module(ModuleNode &node, const bool verbose)
{
    AstPrinter ast_printer(verbose);
    ast_printer.visit(node);
}

const char *opcode_str(const OpCode op)
{
    // clang-format off
    switch (op) {
    case OP_NULL:          return "OP_NULL";
    case OP_TRUE:          return "OP_TRUE";
    case OP_FALSE:         return "OP_FALSE";
    case OP_ADD:           return "OP_ADD";
    case OP_SUB:           return "OP_SUB";
    case OP_MUL:           return "OP_MUL";
    case OP_DIV:           return "OP_DIV";
    case OP_FLOORDIV:      return "OP_FLOORDIV";
    case OP_MOD:           return "OP_MOD";
    case OP_LT:            return "OP_LT";
    case OP_LEQ:           return "OP_LEQ";
    case OP_GT:            return "OP_GT";
    case OP_GEQ:           return "OP_GEQ";
    case OP_EQEQ:          return "OP_EQEQ";
    case OP_NEQ:           return "OP_NEQ";
    case OP_NEGATE:        return "OP_NEGATE";
    case OP_NOT:           return "OP_NOT";
    case OP_LIST:          return "OP_LIST";
    case OP_CLOSURE:       return "OP_CLOSURE";
    case OP_CLASS:         return "OP_CLASS";
    case OP_METHOD:        return "OP_METHOD";
    case OP_GET_CONST:     return "OP_GET_CONST";
    case OP_GET_LOCAL:     return "OP_GET_LOCAL";
    case OP_SET_LOCAL:     return "OP_SET_LOCAL";
    case OP_HEAPVAL:       return "OP_HEAPVAL";
    case OP_GET_HEAPVAL:   return "OP_GET_HEAPVAL";
    case OP_SET_HEAPVAL:   return "OP_SET_HEAPVAL";
    case OP_GET_CAPTURED:  return "OP_GET_CAPTURED";
    case OP_SET_CAPTURED:  return "OP_SET_CAPTURED";
    case OP_GET_GLOBAL:    return "OP_GET_GLOBAL";
    // TEMP remove globals when we added user-defined classes
    case OP_SET_GLOBAL:    return "OP_SET_GLOBAL";
    case OP_GET_SUBSCR:    return "OP_GET_SUBSCR";
    case OP_SET_SUBSCR:    return "OP_SET_SUBSCR";
    case OP_GET_FIELD:     return "OP_GET_FIELD";
    case OP_SET_FIELD:     return "OP_SET_FIELD";
    case OP_GET_METHOD:    return "OP_GET_METHOD";
    case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
    case OP_JUMP_IF_TRUE:  return "OP_JUMP_IF_TRUE";
    case OP_JUMP:          return "OP_JUMP";
    case OP_CALL:          return "OP_CALL";
    case OP_RETURN:        return "OP_RETURN";
    case OP_POP:           return "OP_POP";
    case OP_POP_N:         return "OP_POP_N";
    case OP_PRINT:         return "OP_PRINT";
    default:               return nullptr;
    }
    // clang-format on
}

// TODO currently this instruction is a bit confusing because for constants we print their value
// but for GET/SET we print the index. Would make a bit more sense to print the name of the ident
void disassemble_chunk(const Chunk &chunk, const char *name)
{
    printf("     [disassembly for %s]\n", name);
    for (i32 i = 0; i < chunk.code().len(); i++) {
        printf("%4d | ", i);
        const u8 op = chunk.code()[i];
        printf("%-20s", opcode_str(OpCode(op)));
        switch (op) {
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_HEAPVAL:
        case OP_SET_HEAPVAL:
        case OP_GET_CAPTURED:
        case OP_SET_CAPTURED:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_CALL:
        case OP_POP_N:
        case OP_LIST:
        case OP_HEAPVAL: printf("%d\n", chunk.code()[++i]); break;
        case OP_JUMP_IF_FALSE:
        case OP_JUMP_IF_TRUE:
        case OP_JUMP: {
            const u16 offset = (i += 2, (chunk.code()[i - 1] << 8) | chunk.code()[i]);
            printf("%d\n", offset);
            break;
        }
        case OP_GET_FIELD:
        case OP_SET_FIELD:
        case OP_GET_METHOD:
        case OP_GET_CONST: {
            print_val(chunk.constants()[chunk.code()[++i]]);
            printf("\n");
            break;
        }
        case OP_CLOSURE: {
            const i32 capture_cnt = chunk.code()[++i];
            printf("%d\n", capture_cnt);
            for (i32 j = 0; j < capture_cnt; j++) {
                printf("     | %*s%s\n", 20, "", loc_tag_str(LocTag(chunk.code()[++i])));
                printf("     | %*s%d\n", 20, "", chunk.code()[++i]);
            }
            break;
        }
        default: printf("\n");
        }
    }
    printf("\n");
}

void print_stack(const VM &vm, const Value *sp, const Value *bp)
{
    printf("    [value stack]\n");
    i32 i = 0;
    while (vm.val_stack + i != sp) {
        if (vm.val_stack + i == bp)
            printf("bp > ");
        else
            printf("     ");
        print_val(vm.val_stack[i]);
        printf("\n");
        i++;
    }
    printf("sp >\n\n");
}