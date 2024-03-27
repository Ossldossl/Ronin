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
typedef struct scope_t scope_t;
typedef struct symbol_t symbol_t;

struct trait_t{
    array_t fns;
};

struct type_t {
    array_t traits;
    array_t fields; // array of type_member_t
    u32 size; // size = 0 indicates that the type needs to be finalized
    str_t name;
};

struct type_ref_t {
    bool inferred;
    bool resolved;
    union {
        type_t* resolved_type;
        type_ref_t* rhs;
        expr_t* infer_from;
        str_t ident;
    };
    bool is_array;
    expr_t* array_len; // after typechecking the pointer is the value of the array len or 0
    bool is_ptr;
    span_t loc;
};

typedef struct type_member_t {
    u16 offset;
    str_t name;
    type_ref_t type;
} type_member_t;

struct field_t {
    str_t name;
    type_ref_t type;
    bool is_const;
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
    POST_NULL,
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
    scope_t* scope;
} block_expr_t;

typedef struct arm_t {
    expr_t* condition;
    expr_t* block;
} arm_t;

typedef struct {
    expr_t* val;
    array_t* arms; // array_t of arm_t
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
    field_t* var;
    expr_t* initializer;
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
    bool returns;
    scope_t* scope;
    bool is_inline;
    span_t loc;
} fn_t;

typedef struct {
    map_t cases; // map_t of u64
    u16 case_count;
    str_t name;
} enum_t;

typedef struct {
    array_t members;
    u32 size;
    str_t name;
} union_t;

struct symbol_t {
    enum {
        SYMBOL_TYPE, // structs
        SYMBOL_UNION,
        SYMBOL_ENUM,
        SYMBOL_VAR,
        SYMBOL_FN,
    } kind;
    union {
        type_t _type;
        union_t _union;
        enum_t _enum;
        field_t _var;
        fn_t _fn;
    };
    span_t loc;
};

typedef struct scope_t {
    struct scope_t* parent;
    u16 num_vars;
    map_t syms; // map of symbol_t
} scope_t;

typedef struct {
    str_t ident;
    char* file_path;
    u8 file_id;
} import_t;

typedef struct {
    scope_t* global_scope;
    str_t ident;
    map_t imports; // map_t of file_t
    struct {
        u16 index; u8 len;   
    } imported_files_slice;
} file_t;
    
file_t parser_parse();
void parser_debug(file_t* ast);