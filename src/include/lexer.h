#pragma once
#include <stdint.h>

typedef enum {
    TOKEN_ERR,
    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,
    TOKEN_STR_LIT,

    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }

    TOKEN_PLUS,         // +
    TOKEN_PLUS_EQ,      // +=
    TOKEN_MINUS,        // -
    TOKEN_MINUS_EQ,     // -=
    TOKEN_ASTERISK,     // *
    TOKEN_ASTERISK_EQ,  // *=
    TOKEN_SLASH,        // /
    TOKEN_SLASH_EQ,     // /=
    TOKEN_BOR,          // |
    TOKEN_BOR_EQ,       // |=
    TOKEN_BAND,         // &
    TOKEN_BAND_EQ,      // &=
    TOKEN_BNOT,         // ~
    TOKEN_BNOT_EQ,      // ~=
    TOKEN_XOR,          // ^
    TOKEN_XOR_EQ,       // ^=

    TOKEN_EQ,           // =
    TOKEN_NEQ,          // !=
    TOKEN_GT,           // >
    TOKEN_GEQ,          // >=
    TOKEN_LT,           // <
    TOKEN_LEQ,          // <=

    TOKEN_NOT,          // !
    TOKEN_LOR,          // ||
    TOKEN_LAND,         // &&

    TOKEN_COMMA,        // ,
    TOKEN_PERIOD,       // .
    TOKEN_COLON,        // :
    TOKEN_DCOLON,       // ::

    TOKEN_ARROW,        // =>

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
    TOKEN_RETURN,
    TOKEN_SELFVAL,      // self
    TOKEN_SELFTYPE,     // Self
    TOKEN_STRUCT,
    TOKEN_TRAIT,
    TOKEN_TRUE,
    TOKEN_WHERE,
    TOKEN_WHILE,

    TOKEN_EOF,
} token_type_e;

typedef struct token_t {
    token_type_e type;
} token_t;

uint32_t lexer_tokenize(char* file_content, int file_size);