#include <stdarg.h>
#include <stdio.h>
#define TOKEN_STRINGS_IMPLEMENTATION
#include "lexer.h"

#define LOC(l, c, le) (Span) {.line = (l), .col = (c), .len = (le), .file_id=(lx->file_id)}

extern Compiler compiler;
extern Arena arena;

void make_error(Str8 msg, Span err_loc) {
    Error* err = array_append(&compiler.errors);
    err->err_loc = err_loc; err->err_text = msg; err->hint_text.len = 0;
}

void make_errorf(Span err_loc, const char* format, ...) 
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    
    char* buf = arena_alloc(&arena, 512);
    Str8 result;
    result.data = buf; 
    result.len = vsnprintf_s(buf, 512, 512, format, arg_ptr);
    va_end(arg_ptr);

    Error* err = array_append(&compiler.errors);
    err->err_text = result;
    err->err_loc = err_loc;
}

void make_errorh(Str8 err_msg, Span err_loc, Str8 hint_msg, Span hint_loc) 
{
    Error* err = array_append(&compiler.errors);
    err->err_loc = err_loc; err->err_text = err_msg; 
    err->hint_loc = hint_loc; err->hint_text = hint_msg;
}

void make_errorhf(Str8 err_msg, Span err_loc, Span hint_loc, const char* format, ...) 
{
    Error* err = array_append(&compiler.errors);
    err->err_loc = err_loc; err->err_text = err_msg;

    Str8 hint_msg;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    char* buf = arena_alloc(&arena, 512);
    hint_msg.len = vsnprintf_s(buf, 512, 512, format, arg_ptr);
    hint_msg.data = buf;
    va_end(arg_ptr);
    
    err->hint_loc = hint_loc; err->hint_text = hint_msg; 
}

Token* make_token_nv(Lexer* lx, TokenKind kind, Span loc) {
    Token* result = array_append(&lx->toks);
    result->kind = kind;
    result->loc = loc;
    return result;
}

#define INT_VALUE(val) ((TokenValue){._int = (val)})
#define UINT_VALUE(val) ((TokenValue){._uint = (val)})
#define DOUBLE_VALUE(val) ((TokenValue){._double = (val)})
#define STRING_VALUE(val) ((TokenValue){._str = (val)})
#define BOOL_VALUE(val) ((TokenValue){._bool = (val)})

Token* make_token_v(Lexer* lx, TokenKind kind, Span loc, TokenValue val) {
    Token* result = array_append(&lx->toks);
    result->kind = kind;
    result->loc = loc;
    result->as = val;
    return result;
}

// returns true on EOF
bool advance(Lexer* lx) {
    lx->index++; lx->col++; // expects \r\n as newline, doesn't work without \r
    if (lx->index == lx->content.len) return true;
    return false;
}

void retreat(Lexer* lx) {
    lx->index--; lx->col--;
}

char get_cur(Lexer* lx) {
    return lx->content.data[lx->index];
}

char get_next(Lexer* lx) {
    lx->index++; lx->col++; 
    return lx->content.data[lx->index];
}

char peek(Lexer* lx) {
    //if (lx->index+1 < lx->content.len) {
        return lx->content.data[lx->index];
    //
    return 0;
}

char next_char_sw(Lexer* lx) {
    while (true) {
        char c = lx->content.data[lx->index];
        if (c == '\r') {
            lx->index += 2; lx->line++; lx->col = 0;
        } else if (c == '\n') {
            lx->line++; lx->col = 0; 
        } else if (c != '\x20' && c != '\t' && c != '\v') return c;
        lx->index++;
    }
}
char is_valid_int(char c) {
    return (c >= '0' && c <= '9') || c == '_';
}

inline bool is_valid_ident_char(char c) {
    if (is_valid_int(c)) return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    return false;
}

bool is_valid_hex_digit(char c) 
{
    if (is_valid_int(c)) return true;
    if (c >= 'a' && c <= 'f') return true;    
    if (c >= 'A' && c <= 'F') return true;
    return false;
}

Token* lexer_parse_hex(Lexer* lx)
{
    u32 start_col = lx->col-1;
    int64_t result = 0;
    char c = peek(lx);
    if (!is_valid_hex_digit(c)) {
        advance(lx);
        make_error(const_str("Empty hex literal"), LOC(lx->line, start_col, 2));
        return null;
    }
    while (true) {
        char c = get_next(lx);
        if (is_valid_int(c)) {
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
    return make_token_v(lx, TOKEN_INT_LIT, LOC(lx->line, start_col, lx->col - start_col), INT_VALUE(result));
}

Token* lexer_parse_bin(Lexer* lx)
{
    u32 start_col = lx->col-1;
    int64_t result = 0;
    char c = peek(lx);
    if (c != '0' && c != '1') {
        advance(lx);
        make_error(const_str("Empty binary literal"), LOC(lx->line, start_col, 2));
        return null;
    }
    while (true) {
        char c = get_next(lx);
        if (c == '1') {
            result <<= 1;
            result ++;
        } else if (c == '0') {
            result <<= 1;
        } else if (c != '_') break;
    }
    return make_token_v(lx, TOKEN_INT_LIT, LOC(lx->line, start_col, lx->col - start_col), INT_VALUE(result));
}

static bool lexer_parse_int(Lexer* lx, u64* result)
{
    u64 last = 0;
    u32 start_col = lx->col;
    char c = lx->content.data[lx->index];
    while (true) {
        if (c >= '0' && c <= '9') {
            *result *= 10;
            *result += c - '0';
            if (*result < last) {
                // overflow detected
                make_error(const_str("Integer literal is too big"), LOC(lx->line, start_col, lx->col - start_col));
                return 0;
            }
            last = *result;
        } else if (c != '_') break;
        c = get_next(lx);
    }
    return true;
}

Token* lexer_parse_exponent(Lexer* lx, double result_d, u32 start_col)
{
    char c = get_next(lx);
    if (c == '-') {
        advance(lx);
        if (c == '\0') { make_error(const_str("'e' in float literal has to be fo1llowed by an integer exponent!"), LOC(lx->line, start_col, lx->col-start_col)); return null;}
        u64 exponent;
        bool ok = lexer_parse_int(lx, &exponent);
        if (!ok) return null;
        for (int i = 0; i < exponent; i++) {
            result_d /= 10.f;
        }
        return make_token_v(lx, TOKEN_FLOAT_LIT, LOC(lx->line, lx->col, lx->col - start_col), DOUBLE_VALUE(result_d));
    } else if (c == '+' || (c >= '0' && c <= '9')) {
        if (c == '+') { 
            advance(lx);
            if (c == '\0') { make_error(const_str("'e' in float literal has to be followed by an integer exponent!"), LOC(lx->line, start_col, lx->col-start_col)); return null;}
        }
        u64 exponent;
        bool ok = lexer_parse_int(lx, &exponent);
        if (!ok) return null;
        for (int i = 0; i < exponent; i++) {
            result_d *= 10.f;
        }
        return make_token_v(lx, TOKEN_FLOAT_LIT, LOC(lx->line, start_col, lx->col - start_col), DOUBLE_VALUE(result_d));
    }
    else { make_error(const_str("'e' in float literal has to be followed by an integer exponent!"), LOC(lx->line, start_col, lx->col - start_col)); return null;}
}

Token* lexer_parse_dec(Lexer* lx)
{
    u32 start_col = lx->col;
    u64 result_i;
    bool ok = lexer_parse_int(lx, &result_i);
    if (!ok) return null;
    char c = get_cur(lx);
    if (c == 'e') {
        double result_d = result_i;
        return lexer_parse_exponent(lx, result_d, start_col);
    } else if (c == '.') {
        c = get_next(lx);
        double result_d = result_i; 
        if (c == 'f') {
            advance(lx);
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
                    advance(lx); break;
                } else if (c == 'e') {
                    return lexer_parse_exponent(lx, result_d, start_col);
                }
                else if (c != '_') break;
                c = get_next(lx);    
            }
        } else if (c == 'e') {
            return lexer_parse_exponent(lx, result_d, start_col);
        }
        return make_token_v(lx, TOKEN_FLOAT_LIT, LOC(lx->line, start_col, lx->col - start_col), DOUBLE_VALUE(result_d));
    }
    return make_token_v(lx, TOKEN_INT_LIT, LOC(lx->line, start_col, lx->col - start_col), INT_VALUE(result_i));
}

Token* lexer_parse_number(Lexer* lx)
{
    char c = lx->content.data[lx->index];
    if (c == '0') {
        c = get_next(lx);
        if (c == 'x') {
            return lexer_parse_hex(lx);
        } else if (c == 'b') {
            return lexer_parse_bin(lx);
        } else if (is_valid_int(c)) {
            return lexer_parse_dec(lx);
        } else {
            return make_token_v(lx, TOKEN_INT_LIT, LOC(lx->line, lx->col-1, 1), INT_VALUE(0));
        }
    } else {
        return lexer_parse_dec(lx);
    }
}

Token* lexer_parse_string(Lexer* lx)
{
    Str8 result;
    u32 start_col = lx->col;
    advance(lx); // skip starting "
    result.len = 0; result.data = &lx->content.data[lx->index];
    while (true) {
        char c = lx->content.data[lx->index];
        // TODO: string interpolation
        if (c == '"') {
            advance(lx);
            return make_token_v(
                lx,
                TOKEN_STR_LIT, 
                LOC(lx->line, start_col+1, result.len), 
                STRING_VALUE(result)
            ); 
        }
        if (c == '\0') {
            make_errorh(
                const_str("Expected '\"', got EOF"), 
                LOC(lx->line, lx->col, 1), 
                const_str("This string literal is never closed"), 
                LOC(lx->line, start_col, 1)
            );
            return null;
        }
        if (c == '\r' || c == '\n') {
            make_error(const_str("String literals can not span mutliple lines"), LOC(lx->line, start_col, result.len+1));
            // find closing "
            while (true) {
                char c = next_char_sw(lx);
                if (c == '\0') {
                    return make_token_nv(lx, TOKEN_EOF, LOC(lx->line, lx->col, 1));
                }
                if (c == '\"') {
                    advance(lx);
                    return make_token_v(lx, TOKEN_STR_LIT, LOC(lx->line, start_col, result.len), STRING_VALUE(result));
                }
                advance(lx);
            }
        }
        advance(lx);
        result.len++;
    }
}

Token* parse_identifier(Lexer* lx)
{
    u32 start_col = lx->col;
    char c = get_cur(lx);
    char* start = &lx->content.data[lx->index];
    int len = 0;
    while (is_valid_ident_char(c)) {
        len++;
        c = get_next(lx);
    }
    return make_token_v(lx, TOKEN_IDENT, LOC(lx->line, start_col, lx->col - start_col), STRING_VALUE(make_str(start, len)));
}

inline bool is_valid_keyword_char(char c) {
    return c >= 'a' && c <= 'z';
}

bool keyword_eq(char* str, const char* keyword, u32 len)
{
    for (int i = 0; i < len; i++) {
        char c_l = str[i];
        char c_r = keyword[i];
        if (c_l != c_r || c_l == '\0') return false;
    }
    return true;
}

#define CHECK_AND_MAKE_TOKEN(str, len, tok)                                                         \
    if (keyword_eq(&lx->content.data[lx->index], (str), (len))) {                                    \
        if (!is_valid_ident_char(lx->content.data[lx->index+(len)])) {                               \
            lx->index+=(len); lx->col+=(len);                                                   \
            return make_token_nv(lx, (tok), LOC(lx->line, start_col, len));         \
        }                                                                                           \
    }

Token*parse_identifier_or_keyword(Lexer* lx) 
{
    u32 start_col = lx->col;
    char c = get_cur(lx);
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
            else CHECK_AND_MAKE_TOKEN("impl", 4, TOKEN_IMPL)
            else CHECK_AND_MAKE_TOKEN("import", 6, TOKEN_IMPORT)
        }
        case 'l': {
            CHECK_AND_MAKE_TOKEN("let", 3, TOKEN_LET); 
        }
        case 'm': {
            CHECK_AND_MAKE_TOKEN("match", 5, TOKEN_MATCH); 
        }
        case 'n': {
            CHECK_AND_MAKE_TOKEN("null", 4, TOKEN_NULL);
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
    return parse_identifier(lx);
}

Token* lexer_tokenize_single(Lexer* lx)
{
    if (lx->index >= lx->content.len) { return make_token_nv(lx, TOKEN_EOF, LOC(lx->line, lx->col, 1)); }
    char c = next_char_sw(lx);
    if (c == '\0') return make_token_nv(lx, TOKEN_EOF, LOC(lx->line, lx->col, 1));
    if (c >= '0' && c <= '9') {
        return lexer_parse_number(lx); 
    }
    else if (c == '"') {
        return lexer_parse_string(lx); 
    }
    switch (c) {
        case '(': {
            advance(lx); return make_token_nv(lx, TOKEN_LPAREN, LOC(lx->line, lx->col-1, 1)); 
        }
        case ')': {
            advance(lx); return make_token_nv(lx, TOKEN_RPAREN, LOC(lx->line, lx->col-1, 1)); 
        }
        case '[': {
            advance(lx); return make_token_nv(lx, TOKEN_LBRACKET, LOC(lx->line, lx->col-1, 1)); 
        }
        case ']': {
            advance(lx); return make_token_nv(lx, TOKEN_RBRACKET, LOC(lx->line, lx->col-1, 1)); 
        }
        case '{': {
            advance(lx); return make_token_nv(lx, TOKEN_LBRACE, LOC(lx->line, lx->col-1, 1)); 
        }
        case '}': {
            advance(lx); return make_token_nv(lx, TOKEN_RBRACE, LOC(lx->line, lx->col-1, 1)); 
        }
        case '+': {
            advance(lx);
            if (peek(lx) == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_PLUS_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (peek(lx) == '+') {
                advance(lx);
                return make_token_nv(lx, TOKEN_INC, LOC(lx->line, lx->col-1-1, 2)); 
            } 
            return make_token_nv(lx, TOKEN_PLUS, LOC(lx->line, lx->col-1, 1)); 
        }
        case '-': {
            advance(lx);
            if (peek(lx) == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_MINUS_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (peek(lx) == '-') {
                advance(lx);
                return make_token_nv(lx, TOKEN_DEC, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_MINUS, LOC(lx->line, lx->col-1, 1)); 
        }
        case '*': {
                advance(lx);
            if (peek(lx) == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_ASTERISK_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_ASTERISK, LOC(lx->line, lx->col-1, 1)); 
        }
        case '/': {
            c = peek(lx);
            advance(lx);
            if (c == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_SLASH_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (c == '/') {
                // skip comment until next line
                while (true) {
                    c = get_next(lx);
                    if (c == '\r') {
                        lx->index+=2; lx->col = 1; lx->line++; break;
                    } else if (c == '\n') {
                        lx->col = 1; lx->line++; break;
                    }
                    else if (c == '\0') break;
                }
                return lexer_tokenize_single(lx);
            }
            return make_token_nv(lx, TOKEN_SLASH, LOC(lx->line, lx->col-1, 1)); 
        }
        case '%': {
            advance(lx);
            if (peek(lx) == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_MODULO_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_MODULO, LOC(lx->line, lx->col-1, 1)); 
        }
        case '|': {
            c = peek(lx);
            advance(lx);
            if (c == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_BOR_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (c == '|') {
                advance(lx);
                return make_token_nv(lx, TOKEN_LOR, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_BOR, LOC(lx->line, lx->col-1, 1)); 
        }
        case '&': {
            advance(lx);
            c = peek(lx);
            if (c == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_BAND_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (c == '&') {
                advance(lx);
                return make_token_nv(lx, TOKEN_LAND, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_BAND, LOC(lx->line, lx->col-1, 1)); 
        }
        case '~': {
            advance(lx);
            return make_token_nv(lx, TOKEN_BNOT, LOC(lx->line, lx->col-1, 1)); 
        }
        case '^': {
                advance(lx);
            if (peek(lx) == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_XOR_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } 
            return make_token_nv(lx, TOKEN_XOR, LOC(lx->line, lx->col-1, 1)); 
        }
        case '<': {
            c = peek(lx);
                advance(lx);
            if (c == '<') {
                advance(lx);
                return make_token_nv(lx, TOKEN_LSHIFT, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (c == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_LEQ, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_LT, LOC(lx->line, lx->col-1, 1)); 
        }
        case '>': {
            c = peek(lx);
                advance(lx);
            if (c == '>') {
                advance(lx);
                return make_token_nv(lx, TOKEN_RSHIFT, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (c == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_GEQ, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_GT, LOC(lx->line, lx->col-1, 1)); 
        }
        case '=': {
            c = peek(lx);
            advance(lx);
            if (c == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_EQ, LOC(lx->line, lx->col-1-1, 2)); 
            } else if (c == '>') {
                advance(lx);
                return make_token_nv(lx, TOKEN_ARROW, LOC(lx->line, lx->col-1-1, 2)); 
            };
            return make_token_nv(lx, TOKEN_ASSIGN, LOC(lx->line, lx->col-1, 1)); 
        }
        case '!': {
            advance(lx);
            if (peek(lx) == '=') {
                advance(lx);
                return make_token_nv(lx, TOKEN_NEQ, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_NOT, LOC(lx->line, lx->col-1, 1)); 
        }
        case '?': {
            advance(lx);
            if (peek(lx) == '?') {
                advance(lx);
                return make_token_nv(lx, TOKEN_DQUEST, LOC(lx->line, lx->col-1-1, 2)); 
            }
            return make_token_nv(lx, TOKEN_QUEST, LOC(lx->line, lx->col-1, 1)); 
        }
        case '.': {
            advance(lx); return make_token_nv(lx, TOKEN_PERIOD, LOC(lx->line, lx->col-1, 1)); 
        }
        case ',': {
            advance(lx); return make_token_nv(lx, TOKEN_COMMA, LOC(lx->line, lx->col-1, 1)); 
        }
        case ';': {
            advance(lx); return make_token_nv(lx, TOKEN_SEMICOLON, LOC(lx->line, lx->col-1, 1)); 
        }
        case ':': {
            advance(lx); return make_token_nv(lx, TOKEN_COLON, LOC(lx->line, lx->col-1, 1)); 
        }
        default: {
            return parse_identifier_or_keyword(lx); 
        }
    }

    lx->index++;
    lx->col++;
    return lexer_tokenize_single(lx);
}

void serialize_toks(Array toks) {
    u32 _count;
    for_array(&toks, Token) 
        printf("%s ", token_type_strings[e->kind]);
    }
}

Array lexer_lex_str(Str8 str, u16 file_id) {
    Lexer lx = {0};
    lx.line = 1; lx.content = str; lx.file_id = file_id;
    lx.toks = array_init(sizeof(Token));

    Token* last_tok = null;
    do {
        last_tok = lexer_tokenize_single(&lx);
    } while (last_tok->kind != TOKEN_EOF);

    serialize_toks(lx.toks);

    return lx.toks;
}
