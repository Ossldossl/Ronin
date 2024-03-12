#pragma once
#include <stdint.h>
#include "misc.h"
#include "str.h"

typedef enum token_type_e token_type_e;

typedef union {
    int64_t int_value;
    double double_value;
    str_t string_value;
} token_value_u;

typedef struct token_t {
    token_type_e type;
    span_t span;
    token_value_u value;
} token_t;

uint32_t lexer_tokenize(char* file_content, size_t file_size, uint8_t file_id);
void lexer_debug(token_t* start, uint32_t token_count);

enum token_type_e {
    TOKEN_ERR,
    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,
    TOKEN_STR_LIT,
    TOKEN_IDENT,

    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }

    TOKEN_PLUS,         // +
    TOKEN_INC,          // ++
    TOKEN_MINUS,        // -
    TOKEN_DEC,          // --
    TOKEN_ASTERISK,     // *
    TOKEN_SLASH,        // /
    TOKEN_MODULO,       // %
    TOKEN_BOR,          // |
    TOKEN_BAND,         // &
    TOKEN_BNOT,         // ~
    TOKEN_XOR,          // ^
    TOKEN_LSHIFT,       // <<
    TOKEN_RSHIFT,       // >>

    TOKEN_PLUS_EQ,      // +=
    TOKEN_MINUS_EQ,     // -=
    TOKEN_ASTERISK_EQ,  // *=
    TOKEN_SLASH_EQ,     // /=
    TOKEN_MODULO_EQ,    // %=
    TOKEN_BOR_EQ,       // |=
    TOKEN_BAND_EQ,      // &=
    TOKEN_XOR_EQ,       // ^=

    TOKEN_ASSIGN,       // =
    TOKEN_EQ,           // ==
    TOKEN_NEQ,          // !=
    TOKEN_LT,           // <
    TOKEN_GT,           // >
    TOKEN_LEQ,          // <=
    TOKEN_GEQ,          // >=

    TOKEN_NOT,          // !
    TOKEN_LOR,          // ||
    TOKEN_LAND,         // &&

    TOKEN_PERIOD,       // .
    TOKEN_COMMA,        // ,
    TOKEN_SEMICOLON,    // ;
    TOKEN_COLON,        // :

    TOKEN_ARROW,        // =>

    TOKEN_QUEST,
    TOKEN_DQUEST,

    // keywords
    TOKEN_AS,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_FALSE,
    TOKEN_FN,
    TOKEN_FOREIGN,
    TOKEN_FOR,
    TOKEN_IF,
    TOKEN_IN,
    TOKEN_IMPL,
    TOKEN_IMPORT,
    TOKEN_LET,
    TOKEN_MATCH,
    TOKEN_PACKAGE,
    TOKEN_RETURN,
    TOKEN_SELFVAL,      // self
    TOKEN_STRUCT,
    TOKEN_SELFTYPE,     // Self
    TOKEN_TRAIT,
    TOKEN_TRUE,
    TOKEN_WHERE,
    TOKEN_WHILE,
    TOKEN_YIELD,

    TOKEN_EOF,
};

#ifdef TOKEN_STRINGS_IMPLEMENTATION
const char* token_type_strings[] = {
    "TOKEN_ERR",
    "TOKEN_INT_LIT",
    "TOKEN_FLOAT_LIT",
    "TOKEN_STR_LIT",
    "TOKEN_IDENT",

    "TOKEN_LPAREN",       // (
    "TOKEN_RPAREN",       // )
    "TOKEN_LBRACKET",     // [
    "TOKEN_RBRACKET",     // ]
    "TOKEN_LBRACE",       // {
    "TOKEN_RBRACE",       // }

    "TOKEN_PLUS",         // +
    "TOKEN_INC",          // ++
    "TOKEN_MINUS",        // -
    "TOKEN_DEC",          // --
    "TOKEN_ASTERISK",     // *
    "TOKEN_SLASH",        // /
    "TOKEN_MODULO",       // %
    "TOKEN_BOR",          // |
    "TOKEN_BAND",         // &
    "TOKEN_BNOT",         // ~
    "TOKEN_XOR",          // ^
    "TOKEN_LSHIFT",       // <<
    "TOKEN_RSHIFT",       // >>

    "TOKEN_PLUS_EQ",      // +=
    "TOKEN_MINUS_EQ",     // -=
    "TOKEN_ASTERISK_EQ",  // *=
    "TOKEN_SLASH_EQ",     // /=
    "TOKEN_MODULO_EQ",    // %=
    "TOKEN_BOR_EQ",       // |=
    "TOKEN_BAND_EQ",      // &=
    "TOKEN_XOR_EQ",       // ^=

    "TOKEN_ASSIGN",       // =
    "TOKEN_EQ",           // ==
    "TOKEN_NEQ",          // !=
    "TOKEN_LT",           // <
    "TOKEN_GT",           // >
    "TOKEN_LEQ",          // <=
    "TOKEN_GEQ",          // >=

    "TOKEN_NOT",          // !
    "TOKEN_LOR",          // ||
    "TOKEN_LAND",         // &&

    "TOKEN_PERIOD",       // .
    "TOKEN_COMMA",        "// ",
    "TOKEN_SEMICOLON",    // ;
    "TOKEN_COLON",        // :

    "TOKEN_ARROW",        // =>

    "TOKEN_QUEST",
    "TOKEN_DQUEST",

    // keywords
    "TOKEN_AS",
    "TOKEN_BREAK",
    "TOKEN_CONTINUE",
    "TOKEN_ELSE",
    "TOKEN_ENUM",
    "TOKEN_FALSE",
    "TOKEN_FN",
    "TOKEN_FOREIGN",
    "TOKEN_FOR",
    "TOKEN_IF",
    "TOKEN_IN",
    "TOKEN_IMPL",
    "TOKEN_IMPORT",
    "TOKEN_LET",
    "TOKEN_MATCH",
    "TOKEN_PACKAGE",
    "TOKEN_RETURN",
    "TOKEN_SELFVAL",      // self
    "TOKEN_STRUCT",
    "TOKEN_SELFTYPE",     // Self
    "TOKEN_TRAIT",
    "TOKEN_TRUE",
    "TOKEN_WHERE",
    "TOKEN_WHILE",
    "TOKEN_YIELD",

    "TOKEN_EOF",
};
#endif