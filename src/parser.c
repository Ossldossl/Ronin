#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STRINGS_IMPLEMENTATION
#include "include/misc.h"
#include "include/allocators.h"
#include "include/array.h"
#include "include/map.h"
#include "include/console.h"
#include "include/parser.h"

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
map_t types;

stmt_t* parse_stmt(void);
expr_t* parse_expr(void);

static void print_indent(int indent)
{
    for (int i = 0; i < indent*4; i++) {
        printf(" ");
    }
}

static void print_post_value(int indent, post_expr_t* post)
{
    printf("- val_type: %s\n", post_val_kind_strings[post->val_kind]);
    print_indent(indent+1);
    if (post->val_kind == POST_INT) {
        printf("- value: %lld\n", post->value.int_value);
    } else if (post->val_kind == POST_FLOAT) {
        printf("- value: %lf\n", post->value.double_value);
    } else if (post->val_kind == POST_STR) {
        printf("- value: \"%s\"\n", str_to_cstr(&post->value.string_value));
        arena_free_last(&arena);
    } else if (post->val_kind == POST_IDENT) {
        printf("- ident: %s\n", str_to_cstr(&post->value.string_value));
        arena_free_last(&arena);
    } else if (post->val_kind == POST_TRUE || post->val_kind == POST_FALSE) {
        printf("\r");
    }
}

static void print_statement(u32 indent, stmt_t* stmt);
static void print_expr(int indent, expr_t* expr);

static void print_type(u32 indent, type_ref_t* type)
{
    if (type->inferred) {
        print_indent(indent);
        printf("- (to be inferred)\n");
        return;
    }    

    print_indent(indent);
    if (type->resolved) {
        printf("- resolved\n");
    } else if (!type->is_array) {
        printf("- %s (to be resolved)\n", str_to_cstr(&type->ident));
    } else printf("\r");

    if (type->is_array) {
        print_indent(indent);
        printf("- array of:\n");
        indent++;
            print_type(indent, type->rhs);
        --indent; 
        print_indent(indent);
        printf("- array_len:\n");
        indent++;
            print_expr(indent, type->array_len);
        indent--;
    }

    print_indent(indent);
    printf("- weak?: %s\n", type->is_weak ? "true" : "false");
}

static void print_expr(int indent, expr_t* expr)
{
    if (expr == null) return;
    print_indent(indent);
    switch (expr->kind) {
        case (EXPR_BINARY): {
            printf("- Binary Expression:\n");
            print_indent(indent+1);
            printf("- type: %s\n", bin_kind_strings[expr->bin.kind]);
            print_expr(indent+1, expr->bin.lhs);
            print_expr(indent+1, expr->bin.rhs);
        } break;
        case (EXPR_POST): {
            printf("- %s:\n", expr->post.op_kind == POST_NONE ? "Literal" : "Postfix expr");
            print_indent(indent+1);
            printf("- op_type: %s\n", post_op_kind_strings[expr->post.op_kind]);
            print_indent(indent+1);
            if (expr->post.op_kind == POST_NONE || expr->post.op_kind == POST_INC || expr->post.op_kind == POST_DEC) {
                print_post_value(indent, &expr->post); return;
            } else if (expr->post.op_kind == POST_MEMBER_ACCESS) {
                print_post_value(indent, &expr->post);
                print_indent(indent+1);
                printf("- lhs: \n"); print_expr(indent+2, expr->post.lhs);
            } else if (expr->post.op_kind == POST_ARRAY_ACCESS) {
                printf("- array_index:\n"); print_indent(indent+2); print_post_value(indent+1, &expr->post.array_index->post);
                print_indent(indent+1);
                printf("- lhs:\n"); 
                print_expr(indent+2, expr->post.lhs);
            } else if (expr->post.op_kind == POST_FN_CALL) {
                printf("- args: \n"); 
                indent++;
                u32 _count;
                for_array(&expr->post.args, expr_t*) 
                    print_expr(indent+1, *e);
                }
                if (expr->post.args.used == 0) {print_indent(indent+2); printf("null\n");}
                indent--;
                print_indent(indent+1);
                printf("- lhs:\n");
                print_expr(indent+2, expr->post.lhs);
            }
        } break;
        case (EXPR_UNARY): {
            printf("- Unary Expression:\n");
            print_indent(indent+1);
            printf("- type: %s\n", unary_kind_strings[expr->un.kind]);
            print_expr(indent+1, expr->un.rhs);
        }
        case (EXPR_BLOCK): {
            printf("- Block Expression:\n");
            indent++;
            for_to(i, expr->block.stmts.used) {
                stmt_t** stmt = array_get(&expr->block.stmts, i);
                print_statement(indent, *stmt);
            } 
        } break;
        case (EXPR_MATCH): {
            printf("- Match Expression:\n");
            print_indent(++indent);
                printf("- value to match: ");
                ++indent;
                    print_expr(indent, expr->match.val);
                --indent;
            u32 _count;
            for_array(expr->match.arms, arm_t)
                print_indent(indent);
                printf("- case %d\n", i);
                print_indent(++indent);
                    printf("- condition:\n");
                    ++indent;
                        print_expr(indent, e->condition);
                    print_indent(--indent);
                    printf("- block:\n");
                    indent++;
                        print_expr(indent, e->block);
                    --indent;
                --indent;
            }
        } break;
        case (EXPR_IF): {
            printf("- If Expression:\n");
            print_indent(++indent);
                printf("- condition:\n");
                ++indent;
                    print_expr(indent, expr->if_expr.condition);
                --indent;
                print_indent(indent);
                printf("- body:\n");
                ++indent;
                    print_statement(indent, expr->if_expr.body);
                --indent;
                print_indent(indent);
                printf("- alternative:\n");
                ++indent;
                    print_statement(indent, expr->if_expr.alternative);
                --indent;
        } break;
    }
}

static void print_statement(u32 indent, stmt_t* stmt)
{
    if (stmt == null) return;
    print_indent(indent);
    switch (stmt->type) {
        case STMT_EXPR: {
            printf("\r");
            print_expr(indent, stmt->expr);
        } break;
        case STMT_ASSIGN: {
            printf("- Assign Statement:\n");
            print_indent(++indent);
                printf("- lhs: \n");
                indent++;
                    print_expr(indent, stmt->assign_stmt.lhs);
                indent--;
                print_indent(indent);
                printf("- rhs:\n");
                ++indent;
                    print_expr(indent, stmt->assign_stmt.rhs);
    	        --indent;
            --indent;
        } break;
        case STMT_FOR_LOOP: {
            if (stmt->for_loop.is_for_in) {
                printf("For-in loop:\n");
                print_indent(++indent);
                    printf("- ident: \"%s\"\n", str_to_cstr(&stmt->for_loop.as_for_in.ident));
                    print_indent(indent);
                    printf("- in:\n");
                    ++indent;
                        print_expr(indent, stmt->for_loop.as_for_in.in);
                    --indent;
            } else {
                printf("For loop:\n");
                print_indent(++indent);
                    printf("- Initializer:\n");
                    indent++;
                        print_statement(indent, stmt->for_loop.as_for.initializer);
                    print_indent(--indent);
                    printf("- condition:\n");
                    indent++; 
                        print_expr(indent, stmt->for_loop.as_for.condition);
                    print_indent(--indent);
                    printf("- iteration expression:\n");
                    indent++;
                        print_statement(indent, stmt->for_loop.as_for.iter);
                    --indent;
            }
                print_indent(indent);
                printf("- body:\n");
                indent++;
                    print_statement(indent, stmt->for_loop.body);
                --indent;
            --indent;
        } break;
        case STMT_WHILE_LOOP: {
            printf("- while loop:\n");
            print_indent(++indent);
                printf("- condition:\n");
                ++indent;
                    print_expr(indent, stmt->while_loop.condition);
                print_indent(--indent);
                printf("- body:\n");
                ++indent;
                    print_statement(indent, stmt->while_loop.body);
                --indent;
            --indent;
        } break;
        case STMT_RETURN: {
            printf("- return:\n");
            print_indent(++indent);
                printf("- expr:\n");
                indent++;
                    print_expr(indent, stmt->expr);
                indent--;
            indent--;
        } break;
        case STMT_YIELD: {
            printf("- yield:\n");
            print_indent(++indent);
                printf("- expr:\n");
                indent++;
                    print_expr(indent, stmt->expr);
                indent--;
            indent--;
        } break;
        case STMT_LET: {
            printf("- let statement:\n");
            print_indent(++indent);
                char* ident = str_to_cstr(&stmt->let_stmt.ident);
                printf("- ident: %s\n", ident);
                arena_free_last(&arena);
                print_indent(indent);
                printf("- type:\n");
                ++indent;
                    print_type(indent, &stmt->let_stmt.type);
                --indent;
                print_indent(indent);
                printf("- initializer: %s", stmt->let_stmt.initializer==null ? "empty\n" : "\n");
                ++indent;
                    print_expr(indent, stmt->let_stmt.initializer);
                --indent;
            --indent;
        }
    }
}

void print_fn(fn_t* fn)
{
    printf("FUNCTION %s (", str_to_cstr(&fn->name));
    uint32_t _count;
    for_array(&fn->args, field_t)
        printf("%s", str_to_cstr(&e->name));
        if (i < fn->args.used-1) {
            printf(", ");
        }
    }
    printf(")");
    if (!fn->return_type.resolved && !fn->return_type.inferred) {
        printf("-> %s", str_to_cstr(&fn->return_type.ident));
    }
    printf("{\n");

    u32 indent = 1;
    for_array(&fn->body, stmt_t*) 
        print_statement(indent, *e);
    }

    printf("ENDFUNCTION %s\n\n", str_to_cstr(&fn->name));
}

void parser_debug(ast_t* ast)
{
    arena_begin_section(&arena);
    // iterate over all functions in current file
    map_t* cur = map_get_at(&ast->fns, 0); 
    if (cur == null) {
        log_fatal("No functions in file!"); exit(-4);
    }
    do {
        fn_t* fn = cur->value;
        if (fn == null) { log_fatal("fn_t in map should never be null!"); return; }
        print_fn(fn);
    } while ((cur = map_next(cur)));
    arena_end_section(&arena);
}

inline static token_t* get_next(void) 
{
    parser.index++;
    if (parser.index >= parser.token_count) {
        return &parser.tokens[parser.token_count-1];
    }
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

bool expect_semicolon(span_t* belongs_to)
{
    token_t* tok = get_cur();
    if (tok->type != TOKEN_SEMICOLON) {
        if (belongs_to == null) {
            make_error("Missing semicolon after this token", get_previous()->span);
        } else {
            make_error_h("Missing semicolon after this token", get_previous()->span, "The missing semicolon belongs to this token/expression", *belongs_to);
        }
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
    advance();
}

void recover_until_next_rbrace(void)
{
    token_t* cur = get_cur();
    if (cur == null) return retreat();
    while (cur->type != TOKEN_RBRACE) {
        if (cur->type == TOKEN_EOF) break;
        cur = get_next();
    }
    advance();

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

bool is_valid_type_expr(expr_t* expr)
{
    return 
        (expr->kind == EXPR_UNARY && expr->un.kind == UNARY_ARRAY_OF)
     || (expr->kind == EXPR_POST && expr->post.op_kind == POST_IDENT); 
}

expr_t* make_bin_expr(expr_t* lhs, expr_t* rhs, bin_expr_e kind, span_t loc)
{
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind     = EXPR_BINARY;
    result->loc      = loc;
    result->bin.lhs  = lhs;
    result->bin.rhs  = rhs;
    result->bin.kind = kind;
    result->bin.type.inferred = true;
    return result;
}

expr_t* make_un_expr(expr_t* rhs, un_expr_e kind, span_t loc)
{
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind    = EXPR_UNARY;
    result->loc     = loc;
    result->un.rhs  = rhs;
    result->un.kind = kind;
    result->un.type.inferred = true;
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

expr_t* make_post_lhs(post_op_kind_e op_kind, expr_t* lhs, span_t loc)
{
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind             = EXPR_POST;
    result->loc              = loc;
    result->post.lhs         = lhs;
    result->post.op_kind     = op_kind;
    result->post.val_kind    = POST_LHS;
    result->post.array_index = 0;
    return result;
}

void parse_import(void)
{
    // TODO: REDO IMPORT 
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
    while (cur->type == TOKEN_COLON && peek()->type == TOKEN_COLON) {
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
    str_t* new_path = array_append(&parser.ast->import_paths);
    *new_path = result;
} 


//#region

expr_t* parse_post_ident(expr_t* lhs)
{
    if (lhs == null) return lhs;
    token_t* op = get_cur();
    switch (op->type) {
        case TOKEN_INC: { 
            return make_post_lhs(POST_INC, lhs, op->span);
        }
        case TOKEN_DEC: { 
            return make_post_lhs(POST_DEC, lhs, op->span);   
        }
        case TOKEN_LPAREN: { 
            lhs = make_post_lhs(POST_FN_CALL, lhs, lhs->loc);
            lhs->post.args = array_init(sizeof(expr_t*));
            if (get_next()->type == TOKEN_RPAREN) {
                advance();
                return parse_post_ident(lhs);
            }
            do {
                expr_t** arg = array_append(&lhs->post.args);
                *arg = parse_expr();
            } while (match(TOKEN_COMMA));
            token_t* closing = get_cur();
            if (closing->type != TOKEN_RPAREN) {
                make_error_h("No matching closing parenthesis", op->span, "Insert ')' here", closing->span);
                recover_until_next_semicolon(); return null;
            }
            advance();
            return parse_post_ident(lhs);
            break;
        }
        case TOKEN_LBRACKET: { 
            advance();
            expr_t* array_index = parse_expr();
            token_t* closing = get_cur();
            if (closing->type != TOKEN_RBRACKET) {
                make_error_h("No matching closing bracket", op->span, "Put a ']' here", closing->span);
                recover_until_next_semicolon(); return null;
            }
            advance();
            lhs = make_post_lhs(POST_ARRAY_ACCESS, lhs, LOC(op->span.line, op->span.col, closing->span.col - op->span.col));
            lhs->post.array_index = array_index;
            return parse_post_ident(lhs);
            break;
        }  
        case TOKEN_PERIOD: { 
            token_t* member = get_next();
            if (member->type != TOKEN_IDENT) {
                make_error("Expected identifier in member access!", member->span);
                recover_until_next_semicolon(); return null;
            }
            advance();
            expr_t* new_lhs = make_post_expr(&member->value, POST_IDENT, POST_MEMBER_ACCESS, member->span);
            new_lhs->post.lhs = lhs;
            return parse_post_ident(new_lhs);
            break;
        } 
        default: break;
    }
    return lhs;
}

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
    else if (val->type == TOKEN_TRUE) {
        return make_post_expr(&val->value, POST_TRUE, POST_NONE, val->span);
    } 
    else if (val->type == TOKEN_FALSE) {
        return make_post_expr(&val->value, POST_FALSE, POST_NONE, val->span);
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
        else return make_post_expr(&val->value, POST_STR, POST_NONE, val->span);
    } 
    else if (val->type == TOKEN_IDENT) {
        expr_t* lhs = make_post_expr(&val->value, POST_IDENT, POST_NONE, val->span);
        return parse_post_ident(lhs);
    } else if (val->type == TOKEN_LPAREN) {
        expr_t* body = parse_expr();
        token_t* closing = get_cur();
        if (closing->type != TOKEN_RPAREN) {
            make_error_h("No matching closing parenthesis", val->span, "Insert ')' here", closing->span);
            recover_until_next_semicolon(); return null;
        }
        advance(); return body;
    }

    make_error("Unexpected token in expr", val->span);
    recover_until_next_semicolon();
    return null;
}

static un_expr_e parse_unary_op(token_type_e kind)
{
    un_expr_e result = UNARY_NONE;
    switch (kind) {
        case TOKEN_INC:      { result = UNARY_INC;        break; }
        case TOKEN_DEC:      { result = UNARY_DEC;        break; }
        case TOKEN_NOT:      { result = UNARY_LNOT;       break; }
        case TOKEN_BAND:     { result = UNARY_ADDRESS_OF; break; }
        case TOKEN_BNOT:     { result = UNARY_BNOT;       break; }
        case TOKEN_MINUS:    { result = UNARY_NEGATE;     break; }
        case TOKEN_ASTERISK: { result = UNARY_DEREF;      break; }
        default: break;
    }
    return result;
}

expr_t* parse_unary(void)
{
    token_t* op_tok = get_cur();
    un_expr_e kind = parse_unary_op(op_tok->type);
    if (kind == UNARY_NONE) return parse_post();
    advance();
    expr_t* rhs = parse_unary(); 
    return make_un_expr(rhs, kind, op_tok->span);;
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

expr_t* parse_mul(void)
{
    expr_t* lhs = parse_as();
    while (true) {
        token_t* op = get_cur();
        bin_expr_e kind = 0;
        if (op->type == TOKEN_ASTERISK) kind = BINARY_MUL;
        else if (op->type == TOKEN_SLASH) kind = BINARY_DIV;
        else if (op->type == TOKEN_MODULO) kind = BINARY_MOD;
        else break;
        advance();
        expr_t* rhs = parse_as();
        lhs = make_bin_expr(lhs, rhs, kind, op->span);
    }
    return lhs;
}

expr_t* parse_add(void)
{
    expr_t* lhs = parse_mul();
    while (true) {
        token_t* op = get_cur();
        bin_expr_e kind = 0;
        if (op->type == TOKEN_PLUS) kind = BINARY_ADD;
        else if (op->type == TOKEN_MINUS) kind = BINARY_SUB;
        else break;
        advance();
        expr_t* rhs = parse_mul();
        lhs = make_bin_expr(lhs, rhs, kind, op->span);
    }
    return lhs;
}

expr_t* parse_shift(void)
{
    expr_t* lhs = parse_add();
    while (true) {
        token_t* op = get_cur();
        bin_expr_e kind = 0;
        if (op->type == TOKEN_LSHIFT) kind = BINARY_LSHIFT;
        else if (op->type == TOKEN_RSHIFT) kind = BINARY_RSHIFT;
        else break;
        advance();
        expr_t* rhs = parse_add();
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

//#endregion

expr_t* parse_if(void)
{
    token_t* cur = get_next();
    expr_t* result = arena_alloc(&arena, sizeof(expr_t));
    result->kind = EXPR_IF;
    result->if_expr.condition = parse_expr();
    result->if_expr.body = parse_stmt();
    
    cur = get_cur();
    if (cur->type == TOKEN_ELSE) {
        cur = get_next();
        if (cur->type != TOKEN_IF) {
            if (!expect(TOKEN_LBRACE, "Expected either 'if' or a block expression after 'else'")) {
                result->if_expr.alternative = null; 
                recover_until_next_semicolon();
                return result;
            } 
            retreat();
        }
        result->if_expr.alternative = parse_stmt(); 
    }
    return result;
}

expr_t* parse_match(void)
{
    // TODO: parse match
    return null;
}

expr_t* parse_expr(void)
{
    token_t* cur = get_cur();
    if (cur->type == TOKEN_IF) {
        return parse_if();
    }
    else if (cur->type == TOKEN_MATCH) {
        return parse_match();
    }
    else if (cur->type == TOKEN_TRUE) {
        advance();
        return make_post_expr(&cur->value, POST_TRUE, POST_NONE, cur->span);
    } 
    else if (cur->type == TOKEN_FALSE) {
        advance();
        return make_post_expr(&cur->value, POST_FALSE, POST_NONE, cur->span);
    } else if (cur->type == TOKEN_LBRACE) {
        expr_t* result = arena_alloc(&arena, sizeof(expr_t));
        result->kind = EXPR_BLOCK;
        result->loc = cur->span;
        result->block.stmts = array_init(sizeof(stmt_t*));
        cur = get_next(); 
        while (cur->type != TOKEN_RBRACE) {
            if (cur->type == TOKEN_EOF) {
                make_error("Missing closing bracket ('}') after block expression", cur->span); break;
            }
            stmt_t** stmt_slot = array_append(&result->block.stmts);
            *stmt_slot = parse_stmt();
            cur = get_cur();
        }
        advance();
        return result;
    }
    return parse_or();
}

type_ref_t* parse_type(void)
{
    token_t* cur = get_cur();
    if (cur->type == TOKEN_LBRACKET) {
        cur = get_next();
        expr_t* array_len = null;
        if (cur->type != TOKEN_RBRACKET) {
            array_len = parse_expr();
        }
        if (!expect(TOKEN_RBRACKET, "Expected closing bracket after array len")) { return null; }
        type_ref_t* rhs = parse_type();
        type_ref_t* result = arena_alloc(&arena, sizeof(type_ref_t));
        result->is_array = true; result->array_len = array_len;
        result->resolved = rhs->resolved; result->rhs = rhs;
        result->loc = cur->span;
        result->inferred = false;
        return result;
    } 
    else if (cur->type == TOKEN_IDENT) {
        type_ref_t* result = arena_alloc(&arena, sizeof(type_ref_t));
        result->is_weak = false; 
        if (peek()->type == TOKEN_QUEST) {
            advance(); 
            result->is_weak = true; 
        }
        result->is_array = false; result->array_len = null; 
        result->resolved_type = map_gets(&types, cur->value.string_value);
        if (result->resolved_type == null) {
            result->resolved = false;
            result->ident = cur->value.string_value;
        } else result->resolved = true;
        result->loc = cur->span;
        result->inferred = false;
        advance();
        return result; 
    }
    make_error("Expected a type here", cur->span);
    return null;
}

bool parse_type_inline(type_ref_t* result, bool custom_error_message)
{
    token_t* cur = get_cur();
    if (cur->type == TOKEN_LBRACKET) {
        cur = get_next();
        expr_t* array_len = null;
        if (cur->type != TOKEN_RBRACKET) {
            array_len = parse_expr();
        }
        if (!expect(TOKEN_RBRACKET, "Expected closing bracket after array len")) { recover_until_newline(); return false; }
        type_ref_t* rhs = parse_type();
        result->is_array = true; result->array_len = array_len;
        result->resolved = rhs->resolved; result->rhs = rhs;
        result->loc = cur->span;
        result->inferred = false;
        return true;
    } 
    else if (cur->type == TOKEN_IDENT) {
        result->is_weak = false; 
        if (peek()->type == TOKEN_QUEST) {
            advance();
            result->is_weak = true; 
        }
        result->is_array = false; result->array_len = null; 
        result->resolved_type = map_gets(&types, cur->value.string_value);
        if (result->resolved_type == null) {
            result->resolved = false;
            result->ident = cur->value.string_value;
        } else result->resolved = true;
        result->loc = cur->span;
        result->inferred = false;
        advance();
        return true; 
    }
    if (!custom_error_message) make_error("Expected a type here", cur->span);
    result->inferred = true;
    return false;
}

stmt_t* parse_let(void)
{
    token_t* let = get_cur();
    token_t* cur = get_next();
    if (cur->type != TOKEN_IDENT) {
        make_error("Expected identifier after let", cur->span);
        return null;
    }

    stmt_t* let_stmt = arena_alloc(&arena, sizeof(stmt_t));
    let_stmt->type = STMT_LET;
    let_stmt->let_stmt.ident = cur->value.string_value;
    let_stmt->let_stmt.type.inferred = true;
    cur = get_next();
    if (cur->type == TOKEN_COLON) {
        advance();
        bool ok = parse_type_inline(&let_stmt->let_stmt.type, true); 
        if (!ok) {
            make_error_h("Expected a type here", get_previous()->span, "Remove this ':' or provide a type after it", cur->span); 
        } 
        cur = get_cur();    
    }

    let_stmt->let_stmt.initializer = null;
    if (cur->type == TOKEN_ASSIGN) {
        advance();
        let_stmt->let_stmt.initializer = parse_expr(); 
        expect_semicolon(&let->span);
    } else { 
        expect(TOKEN_SEMICOLON, "Expected initalizer ('=') or ';' in let statement"); 
    };
    
    let_stmt->loc = let->span;
    return let_stmt;
}

// TODO: parse for 
stmt_t* parse_for(void)
{
    token_t* f = get_cur();
    token_t* cur = get_next();
    // TODO: parse "for arg, i in args"
    stmt_t* result = arena_alloc(&arena, sizeof(stmt_t));
    result->type = STMT_FOR_LOOP; 
    if (cur->type == TOKEN_IDENT && peek()->type == TOKEN_IN) {
        // for ... in ... loop
        advance(); advance();
        result->for_loop.is_for_in = true;
        result->for_loop.as_for_in.ident = cur->value.string_value;
        result->for_loop.as_for_in.in = parse_expr();
        result->loc = f->span;
        
    } else {
        result->for_loop.is_for_in = false;
        result->for_loop.as_for.initializer = parse_stmt();
        result->for_loop.as_for.condition = parse_expr();
        expect_semicolon(&f->span);
        result->for_loop.as_for.iter = parse_stmt();
    }

    if (!expect(TOKEN_LBRACE, "Expected a block in the for loop")) {
        return result->for_loop.body = null; return result;
    }
    retreat();
    result->for_loop.body = parse_stmt(); 
    return result;
}

stmt_t* parse_while(void)
{
    token_t* w = get_cur(); advance();
    expr_t* cond = parse_expr();
    if (!expect(TOKEN_LBRACE, "Expected block after while statement")) {
        recover_until_next_semicolon();
        return null;
    }
    retreat();
    stmt_t* result = arena_alloc(&arena, sizeof(stmt_t));
    result->type = STMT_WHILE_LOOP; result->loc = w->span; 
    result->while_loop.condition = cond;
    result->while_loop.body = parse_stmt();
    return result;
}

stmt_t* parse_return(bool is_yield)
{
    token_t* ret = get_cur(); advance();
    expr_t* value = parse_expr();
    expect_semicolon(&ret->span);
    stmt_t* return_stmt = arena_alloc(&arena, sizeof(stmt_t));
    return_stmt->type = is_yield ? STMT_YIELD : STMT_RETURN;
    return_stmt->expr = value;
    return_stmt->loc = ret->span;
    return return_stmt;
}

stmt_t* parse_const_decl(token_t* ident)
{
    stmt_t* const_decl_stmt = arena_alloc(&arena, sizeof(stmt_t));
    const_decl_stmt->type = STMT_LET;
    const_decl_stmt->loc = ident->span;
    const_decl_stmt->let_stmt.is_const = true;
    const_decl_stmt->let_stmt.ident = ident->value.string_value;

    token_t* cur = get_next();
    if (cur->type == TOKEN_COLON) {
        // no type provided
        const_decl_stmt->let_stmt.type.inferred = true;
        cur = get_next();
    } else {
        parse_type_inline(&const_decl_stmt->let_stmt.type, false);
        expect(TOKEN_COLON, "Expected ':' after type in constant var declaration");
    }

    const_decl_stmt->let_stmt.initializer = parse_expr();
    return const_decl_stmt;
}

stmt_t* parse_stmt(void) 
{
    token_t* keyword = get_cur();
    if (keyword->type == TOKEN_LET) {
        return parse_let();
    } else if (keyword->type == TOKEN_FOR) {
        return parse_for();
    } else if (keyword->type == TOKEN_WHILE) {
        return parse_while();
    } else if (keyword->type == TOKEN_RETURN) {
        return parse_return(false);
    } else if (keyword->type == TOKEN_YIELD) {
        return parse_return(true);
    } else if (keyword->type == TOKEN_IDENT) {
        token_t* next = get_next(); 
        if (next->type == TOKEN_COLON) {
            return parse_const_decl(keyword);
        }
        // else fallthrough
        retreat();
    } else if (keyword->type == TOKEN_EOF) {
        log_fatal("EOF REACHED!"); return null;
    }
    expr_t* lhs = parse_expr();
    token_t* cur = get_cur();
    stmt_t* result = arena_alloc(&arena, sizeof(stmt_t));
    if (cur->type >= TOKEN_PLUS_EQ && cur->type <= TOKEN_ASSIGN) {
        bin_expr_e kind = cur->type - TOKEN_PLUS_EQ;
        advance();
        expr_t* rhs = parse_expr();
        expr_t* expr = null;
        if (cur->type == TOKEN_ASSIGN) {
            expr = rhs;
        } else {
            expr = make_bin_expr(lhs, rhs, kind, cur->span); 
        }
        result->type = STMT_ASSIGN;
        result->loc = cur->span;
        result->assign_stmt.lhs = lhs;
        result->assign_stmt.rhs = expr;
        expect_semicolon(&cur->span);
    }
    else {
        result->type = STMT_EXPR; result->expr = lhs; result->loc = lhs->loc;
        if (!(result->expr->kind == EXPR_BLOCK || result->expr->kind == EXPR_IF)) {
            expect_semicolon(&lhs->loc);
        }
    }
    return result;
}

fn_t* parse_fn(token_t* ident)
{
    token_t* lparen = get_next();
    if (lparen->type != TOKEN_LPAREN) {
        make_error("Expected argument list after function keyword", get_previous()->span);
        recover_until_next_rbrace(); return null;
    }
    token_t* next = null;
    fn_t* fn = arena_alloc(&arena, sizeof(fn_t));
    fn->args = array_init(sizeof(field_t));
    fn->name = ident->value.string_value;
    fn->loc = ident->span;
    
    if (peek()->type != TOKEN_RPAREN) {
        // parse arguments
        do {
            token_t* ident = get_next();
            if (ident->type != TOKEN_IDENT) {
                make_error("Expected identifier", ident->span); recover_until_next_rbrace(); return null;
            }
            token_t* colon = get_next();
            if (colon->type != TOKEN_COLON) {
                make_error("Expected ':' after argument identifier", colon->span); recover_until_next_rbrace(); return null;
            }

            advance();
            field_t* arg = array_append(&fn->args); 
            arg->name = ident->value.string_value;
            parse_type_inline(&arg->type, false);
            
            next = get_cur();
        } while (next->type == TOKEN_COMMA);
        if (next->type != TOKEN_RPAREN) {
            make_error_h("No matching closing parenthesis", lparen->span, "Insert ')' here", next->span);
            recover_until_next_semicolon(); return null;
        }
    } else { advance(); } 

    next = get_next();
    fn->body = array_init(sizeof(stmt_t*));
    if (next->type == TOKEN_ARROW) {
        // parse return type
        advance();
        parse_type_inline(&fn->return_type, false);
        next = get_cur();
    } 

    if (next->type == TOKEN_SEMICOLON) {
        advance();
        return fn;
    } 
       
    if (next->type != TOKEN_LBRACE) {
        make_error("Expected block or semicolon following function declaration", next->span);
        recover_until_next_semicolon(); return fn;
    }
    next = get_next();
    while (next->type != TOKEN_RBRACE) {
        if (next->type == TOKEN_EOF) {
            make_error("Missing } after function body", next->span); goto end;
        }
        stmt_t** stmt_slot = array_append(&fn->body);
        *stmt_slot = parse_stmt();
        next = get_cur();
    }
    advance();
end:
    return fn;
}

void parse_const_var_decl(token_t* ident)
{
    // TODO: Generics
    token_t* next = get_next();
    if (next->type == TOKEN_FN) {
        fn_t* fn = parse_fn(ident);
        if (fn == null) return;
        if (map_gets(&parser.ast->fns, fn->name) != null) {
            make_error("Function with this name already exists", fn->loc);
            return;
        } 
        map_sets(&parser.ast->fns, fn->name, fn);
    } else if (next->type == TOKEN_STRUCT) {
        //return parse_struct(ident);
        log_fatal("parsing STRUCt is not implemented yet!");
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
        expect_semicolon(&next->span);
    }
}

void parse_impl(token_t* ident) 
{
    return;
}

void parse_toplevel_statement(void) 
{
    token_t* ident = get_cur();
    token_t* next = get_next();
    if (next->type == TOKEN_IMPL) {
        return parse_impl(ident);
    } else if (next->type == TOKEN_COLON && peek()->type == TOKEN_COLON) { 
        advance();
        return parse_const_var_decl(ident); 
    }
    make_error("Expected '::' or 'impl' after identifier in global scope", next->span);
    recover_until_newline();
}

void register_basic_types(void)
{
#define basic_type(ident)   type_t (ident) = {0}; \
                            map_set(&types, #ident, 0, &(ident));
    basic_type(i64)
    basic_type(i32)
    basic_type(i16)
    basic_type(i8)
    basic_type(u64)
    basic_type(u32)
    basic_type(u16)
    basic_type(u8)
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

    types = (map_t){0};
    register_basic_types();

    while (true) {
        token_t* tok = get_cur();
        if (tok->type == TOKEN_EOF || parser.index >= parser.token_count) break;
        else if (tok->type == TOKEN_IMPORT) {
            parse_import();
        }
        else if (tok->type == TOKEN_IDENT) {
            parse_toplevel_statement(); 
        }
        else {
            make_error("unexpected token", tok->span);
            break;
        }
    }
    return parser.ast;
}