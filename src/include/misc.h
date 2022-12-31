#include <Windows.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#define COLOR_RED 31
#define COLOR_YELLOW 33
#define COLOR_WHITE 0
#define COLOR_BLUE 34
#define null NULL

struct arena_allocator;
typedef struct arena_allocator arena_allocator_t;
struct vector;
typedef struct vector vector_t;

typedef struct compiler 
{
	char* file_path;
    char* file_content;
    int   file_size;
    int   index;
    int   col;
    int   line;
    bool  enable_colors;
	vector_t* errors;
	arena_allocator_t* token_t_allocator;
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
	char* content;
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

string_builder_t* stringb_new(int length, char* content);
void stringb_append(string_builder_t* stringb, char* content_to_append, int length_of_appendix);
void stringb_free(string_builder_t* stringb);
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
	TOKEN_FALSE,
	TOKEN_NULL,
	TOKEN_NOT,

	TOKEN_STRING_LITERAL,
	TOKEN_I_NUMBER_LITERAL,
	TOKEN_F_NUMBER_LITERAL,
	TOKEN_COMMENT,

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
	"TOKEN_FALSE",
	"TOKEN_NULL",
	"TOKEN_NOT",

	"TOKEN_STRING_LITERAL",
	"TOKEN_I_NUMBER_LITERAL",
	"TOKEN_F_NUMBER_LITERAL",
	"TOKEN_COMMENT",

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
		int value;
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