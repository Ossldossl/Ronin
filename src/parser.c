#include <stdio.h>
#define STRINGS_IMPLEMENTATION
#include "include/parser.h"
#include "include/arena.h"
#include "include/array.h"
#include "include/misc.h"
#include "include/console.h"
#define STB_DS_IMPLEMENTATION
#include "include/stb_ds.h"

#define LOC(l, c, le) (span_t) {.line = (l), .col = (c), .len = (le), .file_id=(parser.cur_file_id)}

typedef struct {
    token_t* tokens;
    uint32_t token_count;
    uint32_t index;
    uint8_t cur_file_id;
    ast_t* ast;
} parser_t;

extern arena_t arena;
extern array_t errors;

parser_t parser;

static void print_indent(int indent)
{
    for (int i = 0; i < indent*2; i++) {
        printf(" ");
    }
}

static void parser_print_expr(int indent, expr_t* expr)
{
    print_indent(indent);
    if (expr->kind == EXPR_BINARY) {
        printf("- Binary Expression:\n");
        print_indent(indent+1);
        printf("- type: %s\n", bin_kind_strings[expr->bin.kind]);
        parser_print_expr(indent+1, expr->bin.lhs);
        parser_print_expr(indent+1, expr->bin.rhs);
    } else if (expr->kind == EXPR_POST) {
        printf("- Postfix expression:\n");
        print_indent(indent+1);
        printf("- type: %s\n", post_op_kind_strings[expr->post.op_kind]);
        print_indent(indent+1);
        if (expr->post.val_kind == POST_INT) {
            printf("- value: %lld\n", expr->post.value.int_value);
        } else if (expr->post.val_kind == POST_FLOAT) {
            printf("- value: %lf\n", expr->post.value.double_value);
        } else if (expr->post.val_kind == POST_STR) {
            printf("- value: %s\n", str_to_cstr(expr->post.value.string_value));
            arena_free_last(&arena);
        }
    } else if (expr->kind == EXPR_UNARY) {
        printf("- Unary Expression:\n");
        print_indent(indent+1);
        printf("- type: %s\n", unary_kind_strings[expr->un.kind]);
        parser_print_expr(indent+1, expr->un.rhs);
    }
}

void parser_debug(ast_t* ast)
{
    expr_t* expr = ast->test;
    parser_print_expr(0, expr);
}

inline static token_t* get_next(void) 
{
    if (++parser.index >= parser.token_count) return &parser.tokens[parser.token_count-1];
    return &parser.tokens[parser.index];
}

inline static token_t* get_previous(void)
{
    return &parser.tokens[parser.index-1];
}

inline static token_t* get_cur(void) 
{
    if (parser.index >= parser.token_count) return &parser.tokens[parser.token_count-1];
    return &parser.tokens[parser.index];
}

inline static token_t* peek(void)
{
    if (parser.index >= parser.token_count) return &parser.tokens[parser.token_count-1];
    return &parser.tokens[parser.index+1];
}

inline static void advance(void) 
{
    parser.index++;
}

inline static void retreat(void) 
{
    parser.index--;
}

inline static bool match(token_type_e type)
{
    bool result = get_cur()->type == type;
    if (result) advance();
    return result;
}

bool expect(token_type_e expected_type, char* error_msg)
{
    token_t* tok = get_cur();
    if (tok->type != expected_type) {
        make_error(error_msg, tok->span);
        return false;
    }
    advance(); return true;
}

void recover_until_next_semicolon(void)
{
    token_t* cur = get_cur();
    if (cur == null) return retreat();
    while (cur->type != TOKEN_SEMICOLON) {
        if (cur->type == TOKEN_EOF) break;
        cur = get_next();
    }
}

void recover_until_newline(void)
{
    uint32_t last_line;
    token_t* cur = get_cur();
    if (cur == null) return retreat();
    do {
        if (cur->type == TOKEN_EOF) return;
        last_line = cur->span.line;
        cur = get_next();
    } while (cur->span.line != last_line);
}

expr_t* make_bin_expr(expr_t* lhs, expr_t* rhs, bin_expr_e kind, span_t loc)
{
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind     = EXPR_BINARY;
    result->loc      = loc;
    result->bin.lhs  = lhs;
    result->bin.rhs  = rhs;
    result->bin.kind = kind;
    result->bin.type = null;
    return result;
}

expr_t* make_un_expr(expr_t* rhs, un_expr_e kind, span_t loc)
{
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind    = EXPR_UNARY;
    result->loc     = loc;
    result->un.rhs  = rhs;
    result->un.kind = kind;
    result->un.type = null;
    return result;
}

expr_t* make_post_expr(token_value_u* val, post_val_kind_e val_kind, post_op_kind_e op_kind, span_t loc)
{
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind          = EXPR_POST;
    result->loc           = loc;
    result->post.op_kind  = op_kind;
    result->post.val_kind = val_kind;
    result->post.value    = *val;
    return result;
}

void parse_import(void)
{
    token_t* import = get_cur();
    token_t* ident = get_next();
    if (ident->type != TOKEN_IDENT) {
        if (ident->type == TOKEN_STR_LIT) {
            make_error_h("Expected Identifier after import statement", ident->span, "Remove these quotes", LOC(ident->span.line, ident->span.col-1, 1));
        } else {
            make_error("Expected Identifier after import statement", ident->span);
        }
        return recover_until_newline();
    }
    uint8_t len = ident->span.len;
    token_t* cur = get_next();
    while (cur->type == TOKEN_DCOLON) {
        len += cur->span.len;
        cur = get_next();
        if (cur->type != TOKEN_IDENT) {
            make_error("Unexpected token in import-statement", ident->span);
            return recover_until_newline();
        }
        len += cur->span.len;
        cur = get_next();
    }
    str_t path = make_str(ident->value.string_value.data, len);
    str_t result = str_replace2_1(path, "::", '/');
    log_debug("%s => %s", str_to_cstr(path), result.data);
    // TODO: Do something with the import
} 

expr_t* parse_expr(void);

expr_t* parse_post(void)
{
    token_t* val = get_cur();
    token_t* op = get_next();
    if (val->type == TOKEN_INT_LIT || val->type == TOKEN_FLOAT_LIT) {
        post_op_kind_e kind = POST_NONE;
        if      (op->type == TOKEN_INC) { kind = POST_INC; advance(); }
        else if (op->type == TOKEN_DEC) { kind = POST_DEC; advance(); }
        return make_post_expr(&val->value, val->type == TOKEN_INT_LIT ? POST_INT : POST_FLOAT, kind, LOC(val->span.line, val->span.col, kind == POST_NONE ? val->span.len : val->span.col + val->span.len + 2));
    } 
    else if (val->type == TOKEN_STR_LIT) {
        if (op->type == TOKEN_LBRACKET) {
            advance();
            expr_t* idx = parse_expr();
            token_t* closing_bracket = get_cur();
            if (closing_bracket->type != TOKEN_RBRACKET) {
                make_error_h("No matching closing bracket!", op->span, "Put a ']' here", LOC(closing_bracket->span.line, closing_bracket->span.col, 1));
                recover_until_next_semicolon();
                return null;
            }
            return make_post_expr(&val->value, POST_STR, POST_ARRAY_ACCESS, LOC(val->span.line, val->span.col, closing_bracket->span.col - val->span.col));
        }
    } 
    else if (val->type == TOKEN_IDENT) {
        if (op->type == TOKEN_LBRACKET) {
            advance();
            expr_t* idx = parse_expr();
            token_t* closing_bracket = get_cur();
            if (closing_bracket->type != TOKEN_RBRACKET) {
                make_error_h("No matching closing bracket!", op->span, "Put a ']' here", LOC(closing_bracket->span.line, closing_bracket->span.col, 1));
                recover_until_next_semicolon();
                return null;
            }
            expr_t* res =  make_post_expr(&val->value, POST_IDENT, POST_ARRAY_ACCESS, LOC(val->span.line, val->span.col, closing_bracket->span.col - val->span.col));
            res->post.array_index = idx;
            return res;
        } else if (op->type == TOKEN_INC) {
            return make_post_expr(&val->value, POST_IDENT, POST_INC, LOC(val->span.line, val->span.col, val->span.len + 2));
        } else if (op->type == TOKEN_DEC) {
            return make_post_expr(&val->value, POST_IDENT, POST_DEC, LOC(val->span.line, val->span.col, val->span.len + 2));
        } else if (op->type == TOKEN_PERIOD) {
            // TODO: multiple member and array accesses, e.g. test[1][2] or foo.bar.baz
            token_t* member = get_next();
            if (member->type != TOKEN_IDENT) {
                make_error("Expected identifier in member access", member->span);
                recover_until_next_semicolon();
                return null;
            }
            expr_t* res = make_post_expr(&val->value, POST_IDENT, POST_MEMBER_ACCESS, LOC(val->span.line, val->span.col, (val->span.len + member->span.len) + 1));
            res->post.member_ident = member->value.string_value;
            return res;
        } else if (op->type == TOKEN_LPAREN) {
            token_t* cur = get_cur();
            expr_t* res = make_post_expr(&val->value, POST_IDENT, POST_FN_CALL, val->span);
            res->post.args = array_init(sizeof(expr_t*));
            do {
                expr_t** arg = array_append(&res->post.args);
                *arg = parse_expr();
                cur = get_cur();
            } while (cur->type == TOKEN_COMMA);

            if (cur->type != TOKEN_RPAREN) {
                make_error_h("No matching closing parenthesis", op->span, "Insert ')' here", cur->span);
                recover_until_next_semicolon(); return null;
            }
            advance();
            return res;
        }
    }

    make_error("Unexpected token in expr", val->span);
    recover_until_next_semicolon();
    return null;
}

expr_t* parse_unary(void)
{
    expr_t* rhs;
    while (true) {
        token_t* op = get_cur();
        un_expr_e kind = 0;
        if (op->type == TOKEN_LPAREN) {
            advance();
            expr_t* content = parse_expr();
            token_t* closing = get_cur();
            if (closing->type != TOKEN_RPAREN) {
                make_error_h("No matching closing parenthesis", op->span, "Insert ')' here", closing->span);
                recover_until_next_semicolon(); return null;
            }
            advance();
            return content;
        }
        if      (op->type == TOKEN_INC) kind = UNARY_INC;
        else if (op->type == TOKEN_DEC) kind = UNARY_DEC;
        else if (op->type == TOKEN_NOT) kind = UNARY_LNOT;
        else if (op->type == TOKEN_BNOT) kind = UNARY_BNOT;
        else if (op->type == TOKEN_ASTERISK) kind = UNARY_DEREF;
        else if (op->type == TOKEN_MINUS) kind = UNARY_NEGATE;
        else if (op->type == TOKEN_BAND) kind = UNARY_ADDRESS_OF;
        else return parse_post();
        advance();
        rhs = parse_post();
        rhs = make_un_expr(rhs, kind, LOC(op->span.line, op->span.col, get_cur()->span.col - op->span.col));
    }   
    return rhs;
}

expr_t* parse_as(void)
{
    expr_t* lhs = parse_unary();
    while (match(TOKEN_AS)) {
        span_t op_loc = get_previous()->span;
        expr_t* rhs = parse_unary();
        lhs = make_bin_expr(lhs, rhs, BINARY_AS, op_loc);
    }
    return lhs;
}

expr_t* parse_add(void)
{
    expr_t* lhs = parse_as();
    while (true) {
        token_t* op = get_cur();
        bin_expr_e kind = 0;
        if (op->type == TOKEN_PLUS) kind = BINARY_ADD;
        else if (op->type == TOKEN_MINUS) kind = BINARY_SUB;
        else break;
        advance();
        expr_t* rhs = parse_as();
        lhs = make_bin_expr(lhs, rhs, kind, op->span);
    }
    return lhs;
}

expr_t* parse_mul(void)
{
    expr_t* lhs = parse_add();
    while (true) {
        token_t* op = get_cur();
        bin_expr_e kind = 0;
        if (op->type == TOKEN_ASTERISK) kind = BINARY_MUL;
        else if (op->type == TOKEN_SLASH) kind = BINARY_DIV;
        else if (op->type == TOKEN_MODULO) kind = BINARY_MOD;
        else break;
        advance();
        expr_t* rhs = parse_add();
        lhs = make_bin_expr(lhs, rhs, kind, op->span);
    }
    return lhs;
}

expr_t* parse_shift(void)
{
    expr_t* lhs = parse_mul();
    while (true) {
        token_t* op = get_cur();
        bin_expr_e kind = 0;
        if (op->type == TOKEN_LSHIFT) kind = BINARY_LSHIFT;
        else if (op->type == TOKEN_RSHIFT) kind = BINARY_RSHIFT;
        else break;
        advance();
        expr_t* rhs = parse_mul();
        lhs = make_bin_expr(lhs, rhs, kind, op->span);
    }
    return lhs;
}

expr_t* parse_band(void)
{
    expr_t* lhs = parse_shift();
    while (match(TOKEN_BAND)) {
        span_t op_loc = get_previous()->span;
        expr_t* rhs = parse_shift();
        lhs = make_bin_expr(lhs, rhs, BINARY_BAND, op_loc);
    }
    return lhs;
}

expr_t* parse_xor(void)
{
    expr_t* lhs = parse_band();
    while (match(TOKEN_XOR)) {
        span_t op_loc = get_previous()->span;
        expr_t* rhs = parse_band();
        lhs = make_bin_expr(lhs, rhs, BINARY_XOR, op_loc);
    }
    return lhs;
}

expr_t* parse_bor(void)
{
    expr_t* lhs = parse_xor();
    while (match(TOKEN_BOR)) {
        span_t op_loc = get_previous()->span;
        expr_t* rhs = parse_xor();
        lhs = make_bin_expr(lhs, rhs, BINARY_BOR, op_loc);
    }
    return lhs;
}

expr_t* parse_cmp(void)
{
    expr_t* lhs = parse_bor();
    while (true) {
        token_t* op = get_cur();
        if (op->type > TOKEN_GEQ || op->type < TOKEN_EQ) break;
        bin_expr_e type = (op->type - TOKEN_EQ) + BINARY_EQ;
        advance();
        expr_t* rhs = parse_bor();
        lhs = make_bin_expr(lhs, rhs, type, op->span);
    }
    return lhs;
}

expr_t* parse_and(void)
{
    expr_t* lhs = parse_cmp();
    while (match(TOKEN_LAND)) {
        span_t op_loc = get_previous()->span;
        expr_t* rhs = parse_cmp();
        lhs = make_bin_expr(lhs, rhs, BINARY_LAND, op_loc);
    }
    return lhs;
}

expr_t* parse_or(void) 
{
    expr_t* lhs = parse_and();
    while (match(TOKEN_LOR)) {
        span_t op_loc = get_previous()->span;
        expr_t* rhs = parse_and();
        lhs =  make_bin_expr(lhs, rhs, BINARY_LOR, op_loc);
    }
    return lhs;
}

expr_t* parse_expr(void)
{
    expr_t* result = parse_or();
    // TODO: match etc.
    return result;
}

void parse_const_var_decl(token_t* ident)
{
    // TODO: Generics
    token_t* next = get_next();
    if (next->type == TOKEN_FN) {
        //return parse_fn(ident);
        log_fatal("parsing FN is not implemented yet");
        exit(-4);
    } else if (next->type == TOKEN_STRUCT) {
        //return parse_struct(ident);
        log_fatal("parsing STRUCT is not implemented yet");
        exit(-4);
    } else if (next->type == TOKEN_ENUM) {
        //return parse_enum(ident);
        log_fatal("parsing ENUM is not implemented yet");
        exit(-4);
    } else if (next->type == TOKEN_TRAIT) {
        //return parse_trait(ident);
        log_fatal("parsing TRAIT is not implemented yet");
        exit(-4);
    } else {
        // identifier is a constant variable
        expr_t* expr = parse_expr();
        parser.ast->test = expr;
    }
}

void parse_impl(token_t* ident) 
{
    return;
}

void parse_external_statement(void) 
{
    token_t* ident = get_cur();
    token_t* next = get_next();
    if (next->type == TOKEN_IMPL) {
        return parse_impl(ident);
    } else if (next->type == TOKEN_DCOLON) { 
        return parse_const_var_decl(ident); 
    }
    make_error("Expected '::' or 'impl' after identifier in global scope", next->span);
    recover_until_newline();
}

ast_t* parser_parse_tokens(token_t* tokens, uint32_t token_count)
{
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.index = 0;

    parser.ast = arena_alloc(&arena, sizeof(ast_t));
    parser.ast->import_paths = array_init(sizeof(str_t));

    bool ok = expect(TOKEN_PACKAGE, "Every file must start with a package declaration!");
    if (!ok) return parser.ast;

    // TODO: make use of packages
    token_t* package_ident = get_cur();
    ok = expect(TOKEN_IDENT, "Expected identifier after package definition");
    if (!ok) return parser.ast;

    token_t* tok = get_cur();
    while (true) {
        if (tok->type == TOKEN_EOF || parser.index >= parser.token_count) break;
        else if (tok->type == TOKEN_IMPORT) {
            parse_import(); break;
        }
        else if (tok->type == TOKEN_IDENT) {
            parse_external_statement(); break;
        }
        else {
            make_error("unexpected token", tok->span);
            break;
        }
        token_t* tok = get_next();
    }
    return parser.ast;
}