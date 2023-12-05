#pragma once
#include "lexer.h"
#include "stb_ds.h"

// TODO: PARSER


// types: globale hashmap für types
//        alle referenzen (wie return values) sind pointer zu den jeweiligen typen  
typedef struct type_t type_t;
typedef struct expr_t expr_t;
typedef struct stmt_t stmt_t;
typedef struct trait_t trait_t;
typedef struct field_t field_t;

struct trait_t{
    array_t* fns;
};

struct type_t {
    uint16_t trait_count;
    trait_t* implemented_traits; // TODO: Maybe switch to hashmap
    uint8_t fields_count;
    field_t* fields;
};

struct field_t {
    str_t name;
    type_t* type;
};

typedef enum {
    EXPR_POST,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_BLOCK,
    EXPR_MATCH,
    EXPR_IF,
} expr_type_e;

typedef enum {
    POST_INT,
    POST_FLOAT,
    POST_STR,
    POST_IDENT,
    POST_LHS,
} post_val_kind_e;

typedef enum {
    POST_NONE,
    POST_INC,
    POST_DEC,
    POST_FN_CALL,
    POST_ARRAY_ACCESS,  // []
    POST_MEMBER_ACCESS, // .
} post_op_kind_e;

#ifdef STRINGS_IMPLEMENTATION
const char* post_op_kind_strings[] = {
    "POST_NONE",
    "POST_INC",
    "POST_DEC",
    "POST_FN_CALL",
    "POST_ARRAY_ACCESS",  // []
    "POST_MEMBER_ACCESS",
};
#endif

#ifdef STRINGS_IMPLEMENTATION
const char* post_val_kind_strings[] = {
    "POST_INT",
    "POST_FLOAT",
    "POST_STR",
    "POST_IDENT",
    "POST_LHS",
};
#endif

typedef struct {
    post_val_kind_e val_kind;
    post_op_kind_e op_kind;
    union {
        expr_t* array_index;
        array_t args;
    };
    expr_t* lhs;
    token_value_u value;
} post_expr_t;

typedef enum {
    UNARY_NONE,
    UNARY_INC,        // ++a
    UNARY_DEC,        // --a
    UNARY_LNOT,       // !a
    UNARY_BNOT,       // ~a
    UNARY_DEREF,      // *a
    UNARY_NEGATE,     // -a
    UNARY_ADDRESS_OF, // &a
} un_expr_e;

#ifdef STRINGS_IMPLEMENTATION
const char* unary_kind_strings[] = {
    "UNARY_NONE",
    "UNARY_INC",
    "UNARY_DEC",
    "UNARY_LNOT", // logial not
    "UNARY_BNOT", // binary not
    "UNARY_DEREF",
    "UNARY_NEGATE", 
    "UNARY_ADDRESS_OF",
};
#endif

typedef enum {
    BINARY_ADD,
    BINARY_SUB,
    BINARY_MUL,
    BINARY_DIV,
    BINARY_MOD,
    BINARY_BAND,
    BINARY_LAND,
    BINARY_BOR,
    BINARY_LOR,
    BINARY_XOR,
    BINARY_LSHIFT,
    BINARY_RSHIFT,

    BINARY_EQ,
    BINARY_NEQ,
    BINARY_LT,
    BINARY_GT,
    BINARY_LEQ,
    BINARY_GEQ,

    BINARY_AS, // cast
} bin_expr_e;

#ifdef STRINGS_IMPLEMENTATION
const char* bin_kind_strings[] = {
    "BINARY_ADD",
    "BINARY_SUB",
    "BINARY_MUL",
    "BINARY_DIV",
    "BINARY_MOD",
    "BINARY_BAND",
    "BINARY_LAND",
    "BINARY_BOR",
    "BINARY_LOR",
    "BINARY_XOR",
    "BINARY_LSHIFT",
    "BINARY_RSHIFT",

    "BINARY_EQ",
    "BINARY_NEQ",
    "BINARY_LT",
    "BINARY_GT",
    "BINARY_LEQ",
    "BINARY_GEQ",

    "BINARY_AS",
};
#endif

typedef struct {
    un_expr_e kind;
    expr_t* rhs;
    type_t* type;
} un_expr_t;

typedef struct {
    bin_expr_e kind;
    expr_t* lhs;
    expr_t* rhs;
    type_t* type;
} bin_expr_t;

typedef struct {
    size_t stmt_count;
    stmt_t* stmts;
    type_t* yields;
} block_expr_t;

typedef struct {
    expr_t* val;
    uint16_t arms_count;
    struct arms_t {
        expr_t* condition;
        block_expr_t block;
        struct arms_t* next;
    }* arms;
} match_expr_t;

typedef struct {
    expr_t* condition;
    block_expr_t case_true;
    block_expr_t case_false;
} if_expr_t;

struct expr_t {
    expr_type_e kind;
    union {
        un_expr_t un;
        post_expr_t post;
        bin_expr_t bin;
        block_expr_t block;
        match_expr_t match;
        if_expr_t if_expr;
    };
    span_t loc;
};

typedef enum {
    STMT_EXPR,
    STMT_FOR_LOOP,
    STMT_WHILE_LOOP,
    STMT_RETURN,
    STMT_YIELD,
    STMT_LET,
} stmt_type_e;

struct stmt_t {
    stmt_type_e type;
    union {
        expr_t expr;
    };
};

typedef struct {
    str_t name;
    uint8_t args_count;
    field_t* args;
    size_t stmt_count;
    stmt_t* body;
} fn_t;

typedef struct {
    array_t import_paths; // todo
    size_t fn_count;
    fn_t* fns;
    expr_t* test; //remove
} ast_t;
    
ast_t* parser_parse_tokens(token_t* tokens, uint32_t token_count);
void parser_debug(ast_t* ast);