#include <Windows.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#define COLOR_RED 31
#define COLOR_YELLOW 33
#define COLOR_WHITE 0
#define COLOR_BLUE 34
#define null NULL


#define PRINT_DEBUGS_

struct arena_allocator;
typedef struct arena_allocator arena_allocator_t;
struct vector;
typedef struct vector vector_t;
typedef wchar_t rune;
struct node;
typedef struct node node_t;

typedef struct compiler 
{
	char* file_path;
    arena_allocator_t* utf8_file_content;
    int   index;
    int   col;
    int   line;
    bool  enable_colors;
	vector_t* errors;
	arena_allocator_t* token_t_allocator;
	arena_allocator_t* node_allocator;
} compiler_t;

typedef struct error 
{
	bool is_warning;
	int line;
	int col;
	int length;
	char* message;
} error_t;

typedef struct string_builder 
{
	rune* content;
	int length;
} string_builder_t;

#define ASSERT(condition, error) \
if (!(condition)) \
{ \
print_message(COLOR_RED, error); \
__debugbreak(); \
panic("\n"); \
}

#define MAKE_ERROR(is_w, l, c, len, mes)            \
error_t* err             = malloc(sizeof(error_t)); \
         err->is_warning = is_w;                    \
         err->line       = l;                       \
         err->col        = c;                       \
         err->length     = len;                     \
         err->message    = mes;                     \
vector_push(com->errors, err);                      \

#define CHECK_ALLOCATION(var) ASSERT((var) != null, "ASSERTION FAILED: Out of memory!")
#define len_arena(arena) (arena->index)

string_builder_t* stringb_new(int length, rune* content);
void stringb_append(string_builder_t* stringb, rune* content_to_append, int length_of_appendix);
void stringb_free_c(string_builder_t* stringb);
void stringb_free(string_builder_t* stringb);
rune* stringb_to_cstring(string_builder_t* string_b) ;
void print_message(int color, const char* const format, ...);
void print_errors(compiler_t* com);
__declspec(noreturn) void panic(const char* message);
bool init_console(void);
void write_file(const char* file_name, char* content, size_t size);
int open_file(const char* file_name, char** file_content);
HANDLE get_file_handle(const char* file_name, bool write);
bool init_console(void);

// --- vector ---
#pragma region vector
struct vector
{
	void** data;
	int size;
	int used;
	arena_allocator_t* allocator;
};

vector_t* vector_new(void);
void vector_free(vector_t* vector);

void vector_push(vector_t* vector, void* element);
void* vector_pop(vector_t* vector);
void* vector_peek(vector_t* vector, int index);

#define len(vec) (vec)->used
#pragma endregion vector

// --- lexer ---
#pragma region lexer

typedef enum {
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_MOD,
	TOKEN_SEMI,
	TOKEN_LT,      // <
	TOKEN_GT,	   // >

	TOKEN_LPAREN,  // (
	TOKEN_RPAREN,  // )

	TOKEN_SEQ,     // =
	// comparison
	TOKEN_EQ,  	   // ==
	TOKEN_NEQ,     // !=
	TOKEN_LEQ,     // <=
	TOKEN_GEQ,     // >=

	TOKEN_AND,     // &&
	TOKEN_OR,      // ||
	TOKEN_BAND,    // &
	TOKEN_BOR,     // |
	TOKEN_XOR,     // ^
	TOKEN_ARROW,   // ->
	TOKEN_COL,     // :
	TOKEN_FALSE,   // "false"
	TOKEN_true,    // "true"
	TOKEN_NULL,    // "null"
	TOKEN_NOT,     // "not"
	TOKEN_where,   // "where"
	TOKEN_with,    // "with"
	TOKEN_fn,      // "fn"
	TOKEN_if,      // "if"
	TOKEN_while,   // "while"
	TOKEN_for,     // "for"

	TOKEN_STRING_LITERAL,
	TOKEN_I_NUMBER_LITERAL,
	TOKEN_F_NUMBER_LITERAL,
	TOKEN_COMMENT,
	TOKEN_IDENTIFIER,

	TOKEN_ERROR,
	TOKEN_EOF,
	COUNT_TOKENS
} token_type_t;
static const char* token_names[COUNT_TOKENS+1] = {
	"TOKEN_PLUS",
	"TOKEN_MINUS",
	"TOKEN_MUL",
	"TOKEN_DIV",
	"TOKEN_MOD",
	"TOKEN_SEMI",
	"TOKEN_LT",      // <
	"TOKEN_GT",	   // >

	"TOKEN_LPAREN",  // (
	"TOKEN_RPAREN",  // )

	"TOKEN_SEQ",     // =
	// comparison
	"TOKEN_EQ",  	   // ==
	"TOKEN_NEQ",     // !=
	"TOKEN_LEQ",     // <=
	"TOKEN_GEQ",     // >=

	"TOKEN_AND",     // &&
	"TOKEN_OR",      // ||
	"TOKEN_BAND",    // &
	"TOKEN_BOR",     // |
	"TOKEN_XOR",     // ^
	"TOKEN_ARROW",   // ->
	"TOKEN_COL",     // :
	"TOKEN_FALSE",   // "false"
	"TOKEN_true",    // "true"
	"TOKEN_NULL",    // "null"
	"TOKEN_NOT",     // "not"
	"TOKEN_where",   // "where"
	"TOKEN_with",    // "with"
	"TOKEN_fn",      // "fn"
	"TOKEN_if",      // "if"
	"TOKEN_while",   // "while"
	"TOKEN_for",     // "for"

	"TOKEN_STRING_LITERAL",
	"TOKEN_I_NUMBER_LITERAL",
	"TOKEN_F_NUMBER_LITERAL",
	"TOKEN_COMMENT",
	"TOKEN_IDENTIFIER",

	"TOKEN_ERROR",
	"TOKEN_EOF",
	"COUNT_TOKENS",
};

typedef struct token {
    int line;
    int col;
    int length;
	int index;
    token_type_t type;
	union {
		int i_value;
		string_builder_t* s_value;
	};
} token_t;

void lexer_lex_file(compiler_t* com);
#pragma endregion lexer

// --- arena allocator ---
#pragma region arena_allocator
struct arena_allocator {
    void* data;
    int capacity;
    int size_of_type;
    int index;
};

arena_allocator_t* arena_new(int size_of_type, int capacity);
void* arena_alloc(arena_allocator_t* allocator);
void arena_destroy(arena_allocator_t* allocator);
void* arena_get(arena_allocator_t* allocator, int index);
#pragma endregion arena_allocator

// --- parser ---
#pragma region parser
struct node;
typedef struct node node_t;

typedef enum node_type {
	NODE_ROOT,
	NODE_BINARY_EXPR,
	NODE_UNARY_EXPR,
	NODE_I_NUMBER_LITERAL,
} node_type_t;

typedef enum bin_expr_type {
	BIN_ADD,
	BIN_MINUS,
	BIN_MUL,
	BIN_DIV,
	BIN_LT,
	BIN_GT,
	BIN_EQ,
	BIN_NEQ,
	BIN_LEQ,
	BIN_GEQ,
	BIN_AND,
	BIN_OR,
	BIN_BAND,
	BIN_BOR,
	BIN_XOR,
} bin_expr_type_t;
static const char* bin_expr_typenames[15] = {
	"BIN_ADD",
	"BIN_MINUS",
	"BIN_MUL",
	"BIN_DIV",
	"BIN_LT",
	"BIN_GT",
	"BIN_EQ",
	"BIN_NEQ",
	"BIN_LEQ",
	"BIN_GEQ",
	"BIN_AND",
	"BIN_OR",
	"BIN_BAND",
	"BIN_BOR",
	"BIN_XOR",
};

typedef struct node_bin_expr {
	bin_expr_type_t type;
	node_t* lhs;
	node_t* rhs;
	int line;
	int col;
	int length;
} node_bin_expr_t;

typedef enum un_expr_type {
	UN_NEGATE,
	UN_NOT,
} un_expr_type_t;

typedef struct node_unary_expr {
	un_expr_type_t type;
	node_t* rhs;
	int line;
	int col;
	int length;
} node_un_expr_t;

typedef struct node {
	node_type_t type;
	union {
		vector_t* 		  stmts;
		node_un_expr_t    un_expr;
		node_bin_expr_t   bin_expr;
		string_builder_t* s_lit;
		int 			  i_number_lit;
	};
} node_t;

void parser_parse_tokens(compiler_t* com);
#pragma endregion parser

