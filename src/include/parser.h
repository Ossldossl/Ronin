#pragma once
#include "lexer.h"
#include "map.h"

// types: globale hashmap für types
//        alle referenzen (wie return values) sind pointer zu den jeweiligen typen  
typedef struct type_t type_t;
typedef struct type_ref_t type_ref_t;
typedef struct expr_t expr_t;
typedef struct stmt_t stmt_t;
typedef struct trait_t trait_t;
typedef struct field_t field_t;

struct trait_t{
    array_t* fns;
};

// first, type_t* in fns points to a str_t with the identifier of the expected type
// later in type resolution type_t* actually begins to point to a valid type from the map
struct type_t {
    array_t traits;
    array_t fields;
    span_t loc;
};

struct type_ref_t {
    bool inferred;
    bool resolved;
    union {
        type_t* resolved_type;
        type_ref_t* rhs;
        str_t ident;
    };
    bool is_array;
    expr_t* array_len;
    bool is_weak;
    span_t loc;
};

struct field_t {
    str_t name;
    type_ref_t type;
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
    POST_TRUE,
    POST_FALSE,
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
    "POST_TRUE",
    "POST_FALSE",
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
    UNARY_ARRAY_OF, // []i32 / [5]i32
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
    BINARY_BOR,
    BINARY_BAND,
    BINARY_XOR,

    BINARY_LAND,
    BINARY_LOR,
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
    "BINARY_BOR",
    "BINARY_BAND",
    "BINARY_XOR",

    "BINARY_LAND",
    "BINARY_LOR",
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
    type_ref_t type;
    expr_t* array_type_len;
} un_expr_t;

typedef struct {
    bin_expr_e kind;
    expr_t* lhs;
    expr_t* rhs;
    type_ref_t type;
} bin_expr_t;

typedef struct {
    array_t stmts;
} block_expr_t;

typedef struct arms_t {
    expr_t* condition;
    expr_t* block;
} arm_t;

typedef struct {
    expr_t* val;
    array_t* arms;
} match_expr_t;

typedef struct {
    expr_t* condition;
    stmt_t* body;
    stmt_t* alternative;
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
    STMT_ASSIGN,
    STMT_FOR_LOOP,
    STMT_WHILE_LOOP,
    STMT_RETURN,
    STMT_YIELD,
    STMT_LET,
} stmt_type_e;

typedef struct {
    bool is_for_in;
    union {
        struct {
            str_t ident;
            expr_t* in;
        } as_for_in;
        struct {
            stmt_t* initializer;
            expr_t* condition;
            stmt_t* iter;
        } as_for;
    };
    stmt_t* body;
} for_loop_t;

typedef struct {
    expr_t* condition;
    stmt_t* body;
} while_loop_t;

typedef struct {
    expr_t* lhs;
    expr_t* rhs;
} assign_stmt_t;

typedef struct {
    str_t ident;
    type_ref_t type;
    expr_t* initializer;
    bool is_const;
} let_stmt_t;

struct stmt_t {
    stmt_type_e type;
    union {
        expr_t* expr;
        assign_stmt_t assign_stmt;
        for_loop_t for_loop;
        while_loop_t while_loop;
        let_stmt_t let_stmt;
    };
    span_t loc;
};

typedef struct {
    str_t name;
    array_t args;
    array_t body;
    type_ref_t return_type;
    span_t loc;
} fn_t;

typedef struct {
    array_t import_paths; // todo
    map_t fns;
} ast_t;
    
ast_t* parser_parse_tokens(token_t* tokens, uint32_t token_count);
void parser_debug(ast_t* ast);