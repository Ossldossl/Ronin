#include <stdio.h>
#include <string.h>

#define TOKEN_STRINGS_IMPLEMENTATION
#include "include/lexer.h"
#include "include/console.h"
#include "include/allocators.h"
#include "include/array.h"
#include "include/str.h"

#define LOC(f, l, c, le) (span_t) {.line = (l), .col = (c), .len = (le), .file_id=(lexer.cur_file_id)}

typedef struct {
    token_t tok_buf[4];
    u8 cur_tok;
    char* content;
    size_t index;
    u32 line, col;
    u16 cur_file_id;
    size_t size;
} lexer_t;

token_t* lexer_tokenize_single(void);

extern arena_t arena;
extern array_t errors;

lexer_t lexer;

token_t* make_error_token_h(char* error, span_t error_loc, char* hint, span_t hint_loc)
{
    make_error_h(error, error_loc, hint, hint_loc);
    token_t* tok = &lexer.tok_buf[lexer.cur_tok++];
    lexer.cur_tok = (lexer.cur_tok + 1) % 4;
    tok->type = TOKEN_ERR;
    tok->span = error_loc;
    return tok;
}

token_t* make_error_token(char* error, span_t error_loc)
{
    make_error(error, error_loc);
    token_t* tok = &lexer.tok_buf[lexer.cur_tok++];
    lexer.cur_tok = (lexer.cur_tok + 1) % 4;
    tok->type = TOKEN_ERR;
    tok->span = error_loc;
    return tok;
}

// token that has no value associated with it
token_t* make_token_nv(token_type_e type, span_t loc)
{
    //log_debug("added token %s", token_type_strings[type]);
    token_t* tok = &lexer.tok_buf[lexer.cur_tok];
    lexer.cur_tok = (lexer.cur_tok + 1) % 4;
    tok->type = type;
    tok->span = loc;
    return tok;
}

#define INT_VALUE(val) ((token_value_u){.int_value = (val)})
#define UINT_VALUE(val) ((token_value_u){.uint_value = (val)})
#define FLOAT_VALUE(val) ((token_value_u){.float_value = (val)})
#define DOUBLE_VALUE(val) ((token_value_u){.double_value = (val)})
#define STRING_VALUE(val) ((token_value_u){.string_value = (val)})

token_t* make_token_v(token_type_e type, span_t loc, token_value_u val)
{
    token_t* tok = &lexer.tok_buf[lexer.cur_tok];
    lexer.cur_tok = (lexer.cur_tok + 1) % 4;
    tok->type = type;
    tok->span = loc;
    tok->value = val;
    return tok;
}

static inline void advance(void) {
    lexer.index++; lexer.col++; 
}

static inline void retreat(void) {
    lexer.index--; lexer.col--;
}

static inline char get_cur(void) {
    return lexer.content[lexer.index];
}

static inline char get_next(void) {
    lexer.index++; lexer.col++; return lexer.content[lexer.index];
}

static inline char peek(void) {
    return lexer.content[lexer.index+1];
}

char next_char_sw() // skips whitespace
{
    while (true) {
        char c = lexer.content[lexer.index];
        if (c == '\x0D') { // carriage return
            lexer.index++; lexer.line++; lexer.col = 0;
        }
        else if (c == '\x0A') {
            lexer.line++; lexer.col = 0;
        }
        else if (c != '\x20' && c != '\t' && c != '\v') return c;
        lexer.index++;
        lexer.col++;
    }
}

bool is_valid_int_digit(char c) 
{
    return (c >= '0' && c <= '9') || c == '_';
}

bool is_valid_hex_digit(char c) 
{
    if (is_valid_int_digit(c)) return true;
    if (c >= 'a' && c <= 'f') return true;    
    if (c >= 'A' && c <= 'F') return true;
    return false;
}

token_t*lexer_parse_hex(void)
{
    uint32_t start_col = lexer.col-1;
    int64_t result = 0;
    char c = peek();
    if (!is_valid_hex_digit(c)) {
        advance();
        return make_error_token("Empty hex literal", LOC(lexer.cur_file_id, lexer.line, start_col, 2));
    }
    while (true) {
        char c = get_next();
        if (is_valid_int_digit(c)) {
            result *= 16;
            result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            result *= 16;
            result += (c - 'a') + 10;
        } else if (c >= 'A' && c <= 'F') {
            result *= 16;
            result += (c - 'A') + 10;
        } else if (c != '_') break;
    }
    return make_token_v(TOKEN_INT_LIT, LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col), INT_VALUE(result));
}

token_t*lexer_parse_oct(void)
{
    return make_error_token("Parsing octal literals is not implemented yet!", LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 2));
}

token_t*lexer_parse_bin(void)
{
    uint32_t start_col = lexer.col-1;
    int64_t result = 0;
    char c = peek();
    if (c != '0' && c != '1') {
        advance();
        return make_error_token("Empty binary literal", LOC(lexer.cur_file_id, lexer.line, start_col, 2));
    }
    while (true) {
        char c = get_next();
        if (c == '1') {
            result <<= 1;
            result ++;
        } else if (c == '0') {
            result <<= 1;
        } else if (c != '_') break;
    }
    return make_token_v(TOKEN_INT_LIT, LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col), INT_VALUE(result));
}

static int64_t lexer_parse_int(void)
{
    int64_t result = 0;
    char c = lexer.content[lexer.index];
    while (true) {
        if (c >= '0' && c <= '9') {
            result *= 10;
            result += c - '0';
        } else if (c != '_') break;
        c = get_next();
    }
    return result;
}

token_t*lexer_parse_exponent(double result_d, uint32_t start_col)
{
    char c = get_next();
    if (c == '-') {
        advance();
        if (c == '\0') { return make_error_token("'e' in float literal has to be fo1llowed by an integer exponent!", LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col-start_col)); }
        int64_t exponent = lexer_parse_int();
        for (int i = 0; i < exponent; i++) {
            result_d /= 10.f;
        }
        return make_token_v(TOKEN_FLOAT_LIT, LOC(lexer.cur_file_id, lexer.line, lexer.col, lexer.col - start_col), DOUBLE_VALUE(result_d));
    } else if (c == '+' || (c >= '0' && c <= '9')) {
        if (c == '+') { 
            advance();
            if (c == '\0') { return make_error_token("'e' in float literal has to be followed by an integer exponent!", LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col-start_col)); }
        }
        int64_t exponent = lexer_parse_int();
        for (int i = 0; i < exponent; i++) {
            result_d *= 10.f;
        }
        return make_token_v(TOKEN_FLOAT_LIT, LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col), DOUBLE_VALUE(result_d));
    }
    else { return make_error_token("'e' in float literal has to be followed by an integer exponent!", LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col)); }
}

token_t*lexer_parse_dec(void)
{
    uint32_t start_col = lexer.col;
    int64_t result_i = lexer_parse_int();
    char c = get_cur();
    if (c == 'e') {
        double result_d = result_i;
        return lexer_parse_exponent(result_d, start_col);
    } else if (c == '.') {
        c = get_next();
        double result_d = result_i; 
        if (c == 'f') {
            advance();
        } else if (c >= '0' && c <= '9') {
            uint8_t len = 1;
            while (true) {
                if (c >= '0' && c <= '9') {
                    double digit = c - '0';
                    if (c != '0') {
                        for (int i = 0; i < len; i++) {
                            digit /= 10.f;
                        }
                        result_d += digit;
                    }
                    len++;
                } else if (c == 'f') {
                    advance(); break;
                } else if (c == 'e') {
                    return lexer_parse_exponent(result_d, start_col);
                }
                else if (c != '_') break;
                c = get_next();    
            }
        } else if (c == 'e') {
            return lexer_parse_exponent(result_d, start_col);
        }
        return make_token_v(TOKEN_FLOAT_LIT, LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col), DOUBLE_VALUE(result_d));
    }
    return make_token_v(TOKEN_INT_LIT, LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col), INT_VALUE(result_i));
}

token_t*lexer_parse_number(void)
{
    char c = lexer.content[lexer.index];
    if (c == '0') {
        c = get_next();
        if (c == 'x') {
            return lexer_parse_hex();
        } else if (c == 'o') {
            return lexer_parse_oct();
        } else if (c == 'b') {
            return lexer_parse_bin();
        } else if (is_valid_int_digit(c)) {
            return lexer_parse_dec();
        } else {
            return make_token_v(TOKEN_INT_LIT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1), INT_VALUE(0));
        }
    } else {
        return lexer_parse_dec();
    }
}

token_t*lexer_parse_string(void)
{
    str_t result;
    uint32_t start_col = lexer.col;
    advance(); // skip starting "
    result.len = 0; result.data = &lexer.content[lexer.index];
    while (true) {
        char c = lexer.content[lexer.index];
// TODOOOOOOOOOOOOOOOOOOO: Think of good solution for this
//        if (c == '$' && peek() == '{') {
//            return make_token_v(TOKEN_STR_LIT, LOC(lexer.cur_file_id, lexer.line, start_col+1, result.len), STRING_VALUE(result));
//            return make_token_nv(TOKEN_PLUS, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1));
//            return make_token_nv(TOKEN_LPAREN, LOC(lexer.cur_file_id, lexer.line, lexer.col, 1));
//            advance(); advance();
//
//            i32 braces_to_skip = 0;
//            while (true) {
//                c = get_cur();
//                if (c == '{') { braces_to_skip++; }
//                else if (c == '}') {
//                    braces_to_skip--;
//                    // no more braces left to skip
//                    if (braces_to_skip == -1) { break; }
//                }
//                bool eof = lexer_tokenize_single();
//                if (eof) {
//                    return make_error_h(
//                        "Expected '\"', got EOF", 
//                        LOC(lexer.cur_file_id, lexer.line, lexer.col, 1), 
//                        "This string literal is never closed", 
//                        LOC(lexer.cur_file_id, lexer.line, start_col, 1)
//                    );
//                }
//            }
//            return make_token_nv(TOKEN_RPAREN, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1));
//            return make_token_nv(TOKEN_PLUS, LOC(lexer.cur_file_id, lexer.line, lexer.col, 1));
//            // skip }
//            c = get_next();
//            // make new string
//            result.data = &lexer.content[lexer.index]; result.len = 0;
//        }
        if (c == '"') {
            advance();
            return make_token_v(
                TOKEN_STR_LIT, 
                LOC(lexer.cur_file_id, lexer.line, start_col+1, result.len), 
                STRING_VALUE(result)
            ); 
        }
        if (c == '\0') {
            return make_error_token_h(
                "Expected '\"', got EOF", 
                LOC(lexer.cur_file_id, lexer.line, lexer.col, 1), 
                "This string literal is never closed", 
                LOC(lexer.cur_file_id, lexer.line, start_col, 1)
            );
        }
        if (c == '\r' || c == '\n') {
            make_error("String literals can not span mutliple lines", LOC(lexer.cur_file_id, lexer.line, start_col, result.len+1));
            // find closing "
            while (true) {
                char c = next_char_sw();
                if (c == '\0') {
                    return make_token_nv(TOKEN_EOF, LOC(lexer.cur_file_id, lexer.line, lexer.col, 1));
                }
                if (c == '\"') {
                    advance();
                    return make_token_v(TOKEN_STR_LIT, LOC(lexer.cur_file_id, lexer.line, start_col, result.len), STRING_VALUE(result));
                }
                advance();
            }
        }
        advance();
        result.len++;
    }
}

inline bool is_valid_ident_char(char c) {
    if (is_valid_int_digit(c)) return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    return false;
}

token_t*parse_identifier(void)
{
    uint32_t start_col = lexer.col;
    char c = get_cur();
    char* start = &lexer.content[lexer.index];
    int len = 0;
    while (is_valid_ident_char(c)) {
        len++;
        c = get_next();
    }
    return make_token_v(TOKEN_IDENT, LOC(lexer.cur_file_id, lexer.line, start_col, lexer.col - start_col), STRING_VALUE(make_str(start, len)));
}

inline bool is_valid_keyword_char(char c) {
    return c >= 'a' && c <= 'z';
}

bool keyword_eq(char* str, const char* keyword, uint32_t len)
{
    for (int i = 0; i < len; i++) {
        char c_l = str[i];
        char c_r = keyword[i];
        if (c_l != c_r || c_l == '\0') return false;
    }
    return true;
}

#define CHECK_AND_MAKE_TOKEN(str, len, tok)                                                         \
    if (keyword_eq(&lexer.content[lexer.index], (str), (len))) {                                    \
        if (!is_valid_ident_char(lexer.content[lexer.index+(len)])) {                               \
            lexer.index+=(len); lexer.col+=(len);                                                   \
            return make_token_nv((tok), LOC(lexer.cur_file_id, lexer.line, start_col, len));         \
        }                                                                                           \
    }

token_t*parse_identifier_or_keyword(void) 
{
    uint32_t start_col = lexer.col;
    char c = get_cur();
    switch (c) {
        case 'a': {
            CHECK_AND_MAKE_TOKEN("as", 2, TOKEN_AS); 
        }
        case 'b': {
            CHECK_AND_MAKE_TOKEN("break", 5, TOKEN_BREAK); 
        }
        case 'c': {
            CHECK_AND_MAKE_TOKEN("continue", 8, TOKEN_CONTINUE); 
        }
        case 'e': {
            CHECK_AND_MAKE_TOKEN("else", 4, TOKEN_ELSE)
            else {
                CHECK_AND_MAKE_TOKEN("enum", 4, TOKEN_ENUM); 
            }
        }
        case 'f': {
            CHECK_AND_MAKE_TOKEN("false", 5, TOKEN_FALSE)
            else CHECK_AND_MAKE_TOKEN("fn", 2, TOKEN_FN)
            else CHECK_AND_MAKE_TOKEN("foreign", 7, TOKEN_FOREIGN)
            else CHECK_AND_MAKE_TOKEN("for", 3, TOKEN_FOR);
            
        }
        case 'i': {
            CHECK_AND_MAKE_TOKEN("if", 2, TOKEN_IF)
            else CHECK_AND_MAKE_TOKEN("in", 2, TOKEN_IN) 
            else CHECK_AND_MAKE_TOKEN("inline", 6, TOKEN_INLINE)
            else if (keyword_eq(&lexer.content[lexer.index], ("impl"), (4))) {
                if (!is_valid_ident_char(lexer.content[lexer.index + (4)])) {
                    lexer.index += (4);
                    lexer.col += (4);
                    return make_token_nv((TOKEN_IMPL),
                                         (span_t){.line = (lexer.line),
                                                  .col = (start_col),
                                                  .len = (4),
                                                  .file_id = (lexer.cur_file_id)});
                }
            }
            else if (keyword_eq(&lexer.content[lexer.index], ("import"), (6))) {
                    if (!is_valid_ident_char(
                            lexer.content[lexer.index + (6)])) {
                    lexer.index += (6);
                    lexer.col += (6);
                    return make_token_nv(
                        (TOKEN_IMPORT),
                        (span_t){.line = (lexer.line),
                                 .col = (start_col),
                                 .len = (6),
                                 .file_id = (lexer.cur_file_id)});
                    }
            }
        }
        case 'l': {
            CHECK_AND_MAKE_TOKEN("let", 3, TOKEN_LET); 
        }
        case 'm': {
            CHECK_AND_MAKE_TOKEN("match", 5, TOKEN_MATCH); 
        }
        case 'r': {
            CHECK_AND_MAKE_TOKEN("return", 6, TOKEN_RETURN); 
        }
        case 's': {
            CHECK_AND_MAKE_TOKEN("self", 4, TOKEN_SELFVAL)
            else CHECK_AND_MAKE_TOKEN("struct", 6, TOKEN_STRUCT)
            
        }
        case 'S': {
            CHECK_AND_MAKE_TOKEN("Self", 4, TOKEN_SELFTYPE); 
        }
        case 't': {
            CHECK_AND_MAKE_TOKEN("trait", 5, TOKEN_TRAIT)
            else CHECK_AND_MAKE_TOKEN("true", 4, TOKEN_TRUE)
        }
        case 'w': {
            CHECK_AND_MAKE_TOKEN("where", 5, TOKEN_WHERE)
            else CHECK_AND_MAKE_TOKEN("while", 5, TOKEN_WHILE);
            
        } 
        case 'y': {
            CHECK_AND_MAKE_TOKEN("yield", 5, TOKEN_YIELD); 
        }
    }
    return parse_identifier();
}

token_t*lexer_tokenize_single(void)
{
    if (lexer.index >= lexer.size) { return make_token_nv(TOKEN_EOF, LOC(lexer.cur_file_id, lexer.line, lexer.col, 1)); }
    char c = next_char_sw();
    if (c >= '0' && c <= '9') {
        return lexer_parse_number(); 
    }
    else if (c == '"') {
        return lexer_parse_string(); 
    }
    switch (c) {
        case '(': {
            advance(); return make_token_nv(TOKEN_LPAREN, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case ')': {
            advance(); return make_token_nv(TOKEN_RPAREN, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '[': {
            advance(); return make_token_nv(TOKEN_LBRACKET, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case ']': {
            advance(); return make_token_nv(TOKEN_RBRACKET, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '{': {
            advance(); return make_token_nv(TOKEN_LBRACE, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '}': {
            advance(); return make_token_nv(TOKEN_RBRACE, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '+': {
            advance();
            if (peek() == '=') {
                advance();
                return make_token_nv(TOKEN_PLUS_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (peek() == '+') {
                advance();
                return make_token_nv(TOKEN_INC, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } 
            return make_token_nv(TOKEN_PLUS, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '-': {
            advance();
            if (peek() == '=') {
                advance();
                return make_token_nv(TOKEN_MINUS_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (peek() == '-') {
                advance();
                return make_token_nv(TOKEN_DEC, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_MINUS, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '*': {
                advance();
            if (peek() == '=') {
                advance();
                return make_token_nv(TOKEN_ASTERISK_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_ASTERISK, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '/': {
            c = peek();
            advance();
            if (c == '=') {
                advance();
                return make_token_nv(TOKEN_SLASH_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (c == '/') {
                // skip comment until next line
                while (true) {
                    c = get_next();
                    if (c == '\r') {
                        lexer.index+=2; lexer.col = 1; lexer.line++; break;
                    } else if (c == '\n') {
                        lexer.col = 1; lexer.line++; break;
                    }
                    else if (c == '\0') break;
                }
                return lexer_tokenize_single();
            }
            return make_token_nv(TOKEN_SLASH, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '%': {
            advance();
            if (peek() == '=') {
                advance();
                return make_token_nv(TOKEN_MODULO_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_MODULO, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '|': {
            c = peek();
            advance();
            if (c == '=') {
                advance();
                return make_token_nv(TOKEN_BOR_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (c == '|') {
                advance();
                return make_token_nv(TOKEN_LOR, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_BOR, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '&': {
            advance();
            c = peek();
            if (c == '=') {
                advance();
                return make_token_nv(TOKEN_BAND_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (c == '&') {
                advance();
                return make_token_nv(TOKEN_LAND, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_BAND, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '~': {
            advance();
            return make_token_nv(TOKEN_BNOT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '^': {
                advance();
            if (peek() == '=') {
                advance();
                return make_token_nv(TOKEN_XOR_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } 
            return make_token_nv(TOKEN_XOR, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '<': {
            c = peek();
                advance();
            if (c == '<') {
                advance();
                return make_token_nv(TOKEN_LSHIFT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (c == '=') {
                advance();
                return make_token_nv(TOKEN_LEQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_LT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '>': {
            c = peek();
                advance();
            if (c == '>') {
                advance();
                return make_token_nv(TOKEN_RSHIFT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (c == '=') {
                advance();
                return make_token_nv(TOKEN_GEQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_GT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '=': {
            c = peek();
                advance();
            if (c == '=') {
                advance();
                return make_token_nv(TOKEN_EQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            } else if (c == '>') {
                advance();
                return make_token_nv(TOKEN_ARROW, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            };
            return make_token_nv(TOKEN_ASSIGN, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '!': {
                advance();
            if (peek() == '=') {
                advance();
                return make_token_nv(TOKEN_NEQ, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_NOT, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '?': {
                advance();
            if (peek() == '?') {
                advance();
                return make_token_nv(TOKEN_DQUEST, LOC(lexer.cur_file_id, lexer.line, lexer.col-1-1, 2)); 
            }
            return make_token_nv(TOKEN_QUEST, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case '.': {
            advance(); return make_token_nv(TOKEN_PERIOD, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case ',': {
            advance(); return make_token_nv(TOKEN_COMMA, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case ';': {
            advance(); return make_token_nv(TOKEN_SEMICOLON, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        case ':': {
            advance(); return make_token_nv(TOKEN_COLON, LOC(lexer.cur_file_id, lexer.line, lexer.col-1, 1)); 
        }
        default: {
            return parse_identifier_or_keyword(); 
        }
    }

    lexer.index++;
    lexer.col++;
    return lexer_tokenize_single();
}

void lexer_init(char* file_content, size_t file_size, uint8_t file_id)
{
    lexer.index = 0; lexer.line = 1; lexer.col = 1; lexer.cur_file_id = file_id; lexer.content = file_content; lexer.size = file_size;
}