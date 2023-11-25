#include <stdio.h>

#define TOKEN_STRINGS_IMPLEMENTATION
#include "include/lexer.h"
#include "include/arena.h"
#include "include/array.h"

typedef struct {
    size_t index;
    uint32_t line, col;
    char* content;
    size_t size;
    uint32_t token_count;
} lexer_t;

extern arena_t arena;
extern array_t errors;

lexer_t lexer;

void lexer_debug(token_t* start)
{
    token_t* cur = start;
    int last_line = 1;
    while (true) {
        if (cur->type == TOKEN_EOF) { printf("EOF\n"); break; }
        if (cur->span.line != last_line) {
            last_line = cur->span.line;
            printf("\n");
        }
        printf("%s ", token_type_strings[cur->type]);
        cur++;
    }
}

// token that has no value associated with it
void make_token_nv(token_type_e type, span_t loc)
{
    token_t* tok = arena_alloc(&arena, sizeof(token_t));
    tok->type = type;
    tok->span = loc;
    lexer.token_count++;
}

#define INT_VALUE(val) ((token_value_u){.int_value = (val)})
#define UINT_VALUE(val) ((token_value_u){.uint_value = (val)})
#define FLOAT_VALUE(val) ((token_value_u){.float_value = (val)})
#define DOUBLE_VALUE(val) ((token_value_u){.double_value = (val)})
#define STRING_VALUE(val) ((token_value_u){.string_value = (val)})
#define LOC(l, c, le) (span_t) {.line = (l), .col = (c), .len = (le)}

void make_token_v(token_type_e type, span_t loc, token_value_u val)
{
    token_t* tok = arena_alloc(&arena, sizeof(token_t));
    tok->type = type;
    tok->span = loc;
    tok->value = val;
    lexer.token_count++;
}

static inline void advance(void) {
    lexer.index++; lexer.col++; 
}

char next_char() // does not skip whitespace
{
    return lexer.content[lexer.index++];
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
        else if (c == '\0') { lexer.index--; return '\0'; } // so that we catch it in the main loop (EOF)
        lexer.index++;
        lexer.col++;
    }
}

void lexer_parse_number(void)
{
    return;
}

void lexer_parse_string(void)
{
    str_slice_t result;
    uint32_t start_col = lexer.col;
    advance(); // skip starting "
    result.len = 0; result.data = &lexer.content[lexer.index];
    while (true) {
        char c = lexer.content[lexer.index];
        if (c == '"') {
            make_token_v(
                TOKEN_STR_LIT, 
                LOC(lexer.line, start_col+1, result.len), 
                STRING_VALUE(result)
            ); break;
        }
        if (c == '\0') {
            return make_error_h(
                "Expected '\"', got EOF", 
                LOC(lexer.line, lexer.col, 1), 
                "This string literal is never closed", 
                LOC(lexer.line, start_col, 1)
            );
        }
        if (c == '\r' || c == '\n') {
            make_error("String literals may not span mutliple lines", LOC(lexer.line, start_col, result.len));
            // find closing "
            while (true) {
                char c = next_char_sw();
                if (c == '\0') {
                    return make_error_h(
                        "Expected '\"', got EOF", 
                        LOC(lexer.line, lexer.col, 1), 
                        "This string literal is never closed", 
                        LOC(lexer.line, start_col, 1)
                    );
                }
                if (c == '\"') goto end_parse_string;
                advance();
            }
        }
        advance();
        result.len++;
    }
end_parse_string:
    advance(); // skip closing quote
}

uint32_t lexer_tokenize(char* file_content, size_t file_size)
{
    lexer.index = 0; lexer.line = 1; lexer.col = 1; lexer.content = file_content; lexer.size = file_size;

    while (true) {
        if (lexer.index == file_size) { make_token_nv(TOKEN_EOF, LOC(lexer.line, lexer.col, 1)); break; }
        char c = next_char_sw();
        if (c >= '0' && c <= '9') {
            lexer_parse_number(); continue;
        }
        else if (c == '"') {
            lexer_parse_string(); continue;
        }

        lexer.index++;
        lexer.col++;
    }
    return lexer.token_count;
}