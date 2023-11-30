#include "include/parser.h"
#include "include/arena.h"
#include "include/array.h"
#include "include/misc.h"
#include "include/console.h"

#define LOC(l, c, le) (span_t) {.line = (l), .col = (c), .len = (le), .file_id=(parser.cur_file_id)}

typedef struct {
    token_t* tokens;
    uint32_t token_count;
    uint32_t index;
    uint8_t cur_file_id;
} parser_t;

extern arena_t arena;
extern array_t errors;

parser_t parser;

inline static token_t* get_next(void) 
{
    return &parser.tokens[++parser.index];
}

inline static token_t* get_cur(void) 
{
    return &parser.tokens[parser.index];
}

inline static token_t* peek(void)
{
    return &parser.tokens[parser.index+1];
}

inline static void advance(void) 
{
    parser.index++;
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
    while (cur->type != TOKEN_SEMICOLON) {
        if (cur->type == TOKEN_EOF) break;
        cur = get_next();
    }
}

void recover_until_newline(void)
{
    uint32_t last_line;
    token_t* cur = get_cur();
    do {
        if (cur->type == TOKEN_EOF) return;
        last_line = cur->span.line;
        cur = get_next();
    } while (cur->span.line != last_line);
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

void parse_const_var_decl(void)
{
    // TODO
    return;   
}

void parse_impl(void) 
{
    return;
}

void parse_external_statement(void) 
{
    token_t* ident = get_cur();
    token_t* next = get_next();
    if (next->type == TOKEN_IMPL) {
        return parse_impl();
    }
    return parse_const_var_decl();
}

void parser_parse_tokens(token_t* tokens, uint32_t token_count)
{
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.index = 0;

    bool ok = expect(TOKEN_PACKAGE, "Every file must start with a package declaration!");
    if (!ok) return;

    token_t* tok = get_cur();
    while (true) {
        if (tok->type == TOKEN_EOF || parser.index >= parser.token_count) break;
        switch (tok->type) {
            case TOKEN_IMPORT: {
                parse_import(); break;
            }
            case TOKEN_IDENT: {
                parse_external_statement(); break;
            }
            
        }
        tok = get_next();
    }
}