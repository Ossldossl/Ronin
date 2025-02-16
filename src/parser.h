#pragma once
#include "misc.h"
#include "lexer.h"
#include "map.h"

typedef struct Parser Parser;
typedef struct Module Module;
typedef struct ASTNode ASTNode;
typedef struct Expr Expr;
typedef struct Scope Scope;
typedef struct Stmt Stmt;

[[noreturn]] void print_errors_and_exit(void);
Module* parse_tokens(Array tokens);

struct Parser {
    Array* tokens;
    u64 cur_tok;
    Token* cur;
    Module* cur_mod;
    Scope* cur_scope;
    Map hash_to_str; // maps all hashes to strings for debug purposes
};

struct Module {
    u32 hash;
    Scope* global_scope;
    Map imports; // map of Module*
    u16 file_id;
};

typedef enum {
    EXPR_POST,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_BLOCK,
    EXPR_MATCH,
    EXPR_IF,
} ExprKind;

// ==== POST ====
typedef enum {
    POST_TRUE,
    POST_FALSE,
    POST_NULL,
    POST_INT,
    POST_FLOAT,
    POST_STR,
    POST_IDENT,
    POST_LHS,
} PostValueKind;

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

typedef enum {
    POST_NONE,
    POST_INC,
    POST_DEC,
    POST_FN_CALL,
    POST_ARRAY_ACCESS,  // []
    POST_MEMBER_ACCESS, // .
} PostOpKind;

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

typedef struct {
    PostValueKind val_kind;
    PostOpKind op_kind;
    union {
        Expr* array_index;
        Array args;
    };
    Expr* lhs;
    TokenValue value;
} ExprPost;

// ==== UNARY ==== 

typedef struct TypeRef {
    bool is_ptr;
    union {
        struct TypeRef* ptr;
        u32* type;
    };
    bool is_owned;
} TypeRef;

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
} UnaryKind;

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

typedef struct ExprUnary{
    UnaryKind kind;
    Expr* rhs;
    TypeRef type;
    Expr* array_type_len;
} ExprUnary;

// ==== BINARY ====

typedef enum {
    BINARY_INVALID,
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
    BINARY_MEMBER_ACCESS,

    BINARY_AS, // cast
} BinaryKind;

#ifdef STRINGS_IMPLEMENTATION
const char* bin_kind_strings[] = {
    "BINARY_INVALID,"
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
    "BINARY_MEMBER_ACCESS",

    "BINARY_AS",
};
#endif

typedef struct {
    BinaryKind kind;
    Expr* lhs;
    Expr* rhs;
    TypeRef type;
} ExprBinary;

typedef struct {
    Array stmts;
    Scope* scope;
} ExprBlock;

typedef struct {
    Expr* condition;
    Expr* block;
} Arm;

typedef struct {
    Expr* val;
    Array* arms; // array_t of arm_t
} ExprMatch;

typedef struct {
    Expr* condition;
    Stmt* body;
    Stmt* alternative;
} ExprIf;

struct Expr {
    ExprKind kind;
    union {
        ExprUnary un;
        ExprPost post;
        ExprBinary bin;
        ExprBlock block;
        ExprMatch match;
        ExprIf if_expr;
    };
    Span loc;
};

// === STATEMENTS ===
typedef enum {
    STMT_EXPR,
    STMT_ASSIGN,
    STMT_FOR_LOOP,
    STMT_WHILE_LOOP,
    STMT_RETURN,
    STMT_YIELD,
    STMT_LET,
} StmtKind;

typedef struct {
    bool is_for_in;
    union {
        struct {
            Str8 ident;
            Expr* in;
        } as_for_in;
        struct {
            Stmt* initializer;
            Expr* condition;
            Stmt* iter;
        } as_for;
    };
    Stmt* body;
} StmtFor;

typedef struct {
    Expr* condition;
    Stmt* body;
} StmtWhile;

typedef struct {
    Expr* lhs;
    Expr* rhs;
} StmtAssign;

typedef struct Field {
    Str8 name;
    TypeRef type;
} Field;

typedef struct {
    Field* var;
    Expr* initializer;
} StmtLet;

struct Stmt {
    StmtKind type;
    union {
        Expr* expr;
        StmtAssign assign_stmt;
        StmtFor for_loop;
        StmtWhile while_loop;
        StmtLet let_stmt;
    };
    Span loc;
};

typedef struct {
    Str8 name;
    Array args; // array of field
    Array body; // array of Stmt*
    TypeRef return_type;
    bool returns;
    Scope* scope;
    bool is_inline;
    bool is_foreign;
    Span loc;
} Fn;

typedef struct {
    Map cases; // map_t of u64
    u16 case_count;
    Str8 name;
} Enum;

typedef struct {
    Array fields; // array_of Field
    u32 size; 
    u8 align;

    u32 hash;
} Struct;

typedef struct {
    Array members;
    u32 size;

    u32 hash;
} Union;

typedef struct {
    Str8 ident;
    Array constraints;
} GenericParam;

typedef struct {
    // TODO: traits
} Trait;

typedef enum {
    SYM_FN,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM,
    SYM_TRAIT,
    SYM_EXPR,
} SymKind;

typedef struct Symbol {
    Str8 name;
    SymKind kind;
    union {
        Fn* fn_;
        Struct* struct_;
        Union* union_;
        Enum* enum_;
        Trait* trait_;
        Expr* expr_;
    };
} Symbol;

struct Scope {
    struct Scope* parent;
    Map syms; // map of Symbol
};

typedef struct Import {
    Str8 ident;
    char* file_path;
    u16 file_id;
} Import;