#pragma once
#include "misc.h"
#include "str.h"
#include "array.h"
#include "arena.h"
#include "map.h"

typedef struct Compiler Compiler;
typedef struct Lexer Lexer;
typedef struct Token Token;
typedef struct Span Span;
typedef struct Error Error;

typedef enum TokenKind TokenKind;
typedef union TokenValue TokenValue;

Array lexer_lex_str(Str8 str, u16 file_id);

void make_error(Str8 msg, Span err_loc);
void make_errorf(Span err_loc, const char* format, ...);
void make_errorh(Str8 err_msg, Span err_loc, Str8 hint_msg, Span hint_loc);
void make_errorhf(Str8 err_msg, Span err_loc, Span hint_loc, const char* format, ...);

struct Compiler {
    Array errors; // array of Error
    Map imported_files; // map of Module
    u16 cur_file_id;
};

struct Lexer {
    u32 index; 
    u32 col; 
    u32 line;
    u16 file_id;
    Str8 content;
    Array toks;
};

struct Span {
    u16 file_id; // only 255 files for now
    u32 line;
    u32 col;
    u16 len;
};

union TokenValue {
    double _double;
    i64 _int;
    u64 _uint;
    bool _bool;
    Str8 _str;
};

struct Token {
    Span loc;
    TokenKind kind;
    TokenValue as;
};

struct Error {
    Span err_loc;
    Span hint_loc; // optional
    Str8 err_text;
    Str8 hint_text;
};

#define TOKEN_TYPES \
    X(TOKEN_ERR) \
    X(TOKEN_INT_LIT) \
    X(TOKEN_FLOAT_LIT) \
    X(TOKEN_STR_LIT) \
    X(TOKEN_IDENT) \
    X(TOKEN_LPAREN)       /* (  */ \
    X(TOKEN_RPAREN)       /* )  */ \
    X(TOKEN_LBRACKET)     /* [  */ \
    X(TOKEN_RBRACKET)     /* ]  */ \
    X(TOKEN_LBRACE)       /* {  */ \
    X(TOKEN_RBRACE)       /* }  */ \
    X(TOKEN_PLUS)         /* +  */ \
    X(TOKEN_INC)          /* ++  */ \
    X(TOKEN_MINUS)        /* -  */ \
    X(TOKEN_DEC)          /* --  */ \
    X(TOKEN_ASTERISK)     /* *  */ \
    X(TOKEN_SLASH)        /* /  */ \
    X(TOKEN_MODULO)       /* %  */ \
    X(TOKEN_BOR)          /* |  */ \
    X(TOKEN_BAND)         /* &  */ \
    X(TOKEN_BNOT)         /* ~  */ \
    X(TOKEN_XOR)          /* ^  */ \
    X(TOKEN_LSHIFT)       /* <<  */ \
    X(TOKEN_RSHIFT)       /* >>  */ \
    X(TOKEN_PLUS_EQ)      /* +=  */ \
    X(TOKEN_MINUS_EQ)     /* -=  */ \
    X(TOKEN_ASTERISK_EQ)  /* *=  */ \
    X(TOKEN_SLASH_EQ)     /* /=  */ \
    X(TOKEN_MODULO_EQ)    /* %=  */ \
    X(TOKEN_BOR_EQ)       /* |=  */ \
    X(TOKEN_BAND_EQ)      /* &=  */ \
    X(TOKEN_XOR_EQ)       /* ^=  */ \
    X(TOKEN_ASSIGN)       /* =  */ \
    X(TOKEN_EQ)           /* ==  */ \
    X(TOKEN_NEQ)          /* !=  */ \
    X(TOKEN_LT)           /* <  */ \
    X(TOKEN_GT)           /* >  */ \
    X(TOKEN_LEQ)          /* <=  */ \
    X(TOKEN_GEQ)          /* >=  */ \
    X(TOKEN_NOT)          /* !  */ \
    X(TOKEN_LOR)          /* ||  */ \
    X(TOKEN_LAND)         /* &&  */ \
    X(TOKEN_PERIOD)       /* .  */ \
    X(TOKEN_COMMA)        /*   */ \
    X(TOKEN_SEMICOLON)    /* ;  */ \
    X(TOKEN_COLON)        /* :  */ \
    X(TOKEN_ARROW)        /* =>  */ \
    X(TOKEN_QUEST) \
    X(TOKEN_DQUEST) \
    X(TOKEN_AS) \
    X(TOKEN_BREAK) \
    X(TOKEN_CONTINUE) \
    X(TOKEN_ELSE) \
    X(TOKEN_ENUM) \
    X(TOKEN_FALSE) \
    X(TOKEN_FN) \
    X(TOKEN_FOREIGN) \
    X(TOKEN_FOR) \
    X(TOKEN_IF) \
    X(TOKEN_IN) \
    X(TOKEN_INLINE) \
    X(TOKEN_IMPL) \
    X(TOKEN_IMPORT) \
    X(TOKEN_LET) \
    X(TOKEN_MATCH) \
    X(TOKEN_NULL) \
    X(TOKEN_RETURN) \
    X(TOKEN_SELFVAL)       \
    X(TOKEN_STRUCT) \
    X(TOKEN_SELFTYPE)      \
    X(TOKEN_TRAIT) \
    X(TOKEN_TRUE) \
    X(TOKEN_WHERE) \
    X(TOKEN_WHILE) \
    X(TOKEN_YIELD) \
    X(TOKEN_EOF)

#define X(e) e,
enum TokenKind {
    TOKEN_TYPES
};

#undef X
#define X(e) #e,

#ifdef TOKEN_STRINGS_IMPLEMENTATION
const char* token_type_strings[];
const char* token_type_strings[] = {
    TOKEN_TYPES
};
#endif


#undef X

