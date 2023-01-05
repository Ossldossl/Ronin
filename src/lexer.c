#include "include/misc.h"

#define ADVANCE rune c = advance(com); if ((c) == 0) { print_message(COLOR_RED, "ERROR: Unexpected EOF at %d:%d", com->line, com->col); panic("\n");}
#define no_eof(c) if ((c) == true) { print_message(COLOR_RED, "ERROR: Unexpected EOF at %d:%d", com->line, com->col); panic("\n"); }
#define ADVANCE_OR_RET_TRUE rune c = advance(com); if ((c) == 0) { return true;}
#define ADVANCE_OR_RET rune c = advance(com); if ((c) == 0) { return;}
#define ADVANCE_OR_BREAK rune c = advance(com); if ((c) == 0) { break;}

static rune advance(compiler_t* com)
{
    if (com->index == len_arena(com->utf8_file_content)) {
        return 0; 
    }
    rune* result = arena_get(com->utf8_file_content, com->index);
    com->index++;
    com->col++;
    return *result;
}

static rune peek(compiler_t* com) 
{
    if (com->index == len_arena(com->utf8_file_content)) {
        return 0;
    }
    rune* result = arena_get(com->utf8_file_content, com->index);
    return *result;
}

static bool skip_until_newline(compiler_t* com) 
{
    char ch = 'h';
    ADVANCE_OR_RET_TRUE
    ch = c;
    while (true) {
        if (ch == 13) {
            com->index += 1;
            com->col = 0;
            com->line++;
            return false;
        }
        if (ch == '\n') {
            com->index++;
            com->line++;
            com->col = 0;
            printf("skip: %c", ch);
            return false;
        }
        ADVANCE_OR_RET_TRUE
        ch = c;
    }
}

static int search_whitespace(compiler_t* com) 
{
    int index = com->index;
    rune c = *((rune*)arena_get(com->utf8_file_content, index));
    while (c != '\n' && c != '\x20' && c != '\x09' && c != '\r' && index < len_arena(com->utf8_file_content)) {
        index++;
        c = *((rune*)arena_get(com->utf8_file_content, index));
    }
    if (index == com->index) return com->index;
    return index - 1;
}

static bool is_whitespace(rune c) 
{
    return (c == '\x20' || c == '\x09' || c == '\x0D' || c == '\x0A');
}

static int get_end_of_number(compiler_t* com) 
{
    int index = com->index;
    rune c = *((rune*)arena_get(com->utf8_file_content, index));
    while (!is_whitespace(c) && index < len_arena(com->utf8_file_content) - 1) {
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == ')' || c == ']') {
            break;
        }
        index++;
        c = *((rune*)arena_get(com->utf8_file_content, index));
    }
    if (index == com->index) return com->index;
    return index - 1;
}

// returns true when eof reached
static bool skip_whitespace_or_comment(compiler_t* com) 
{
    ADVANCE
    while (true) {
        if (c == '\x20' || c == '\x09') {
            ADVANCE_OR_RET_TRUE
            continue;
        }
        if (c == '/') {
            ADVANCE_OR_RET_TRUE
            if (c == '/') {

                bool eof = skip_until_newline(com);
                no_eof(eof);
                continue;
            }
        }
        return false;
    }
}

#define TOKEN(len, token_type) token_t* tok = arena_alloc(com->token_t_allocator); \
                          tok->col    = com->col;                                  \
                          tok->line   = com->line;                                 \
                          tok->length = len;                                       \
                          tok->type   = token_type;                                \

#define dToken(next_char, tok1, tok2) c = peek(com);                               \
                                      if (c == next_char) {                        \
                                        TOKEN(2, tok2)                             \
                                        advance(com);                              \
                                        continue;                                  \
                                      }                                            \
                                      TOKEN(1, tok1)                               \
                                      continue;                                    \

static void parse_string_literal(compiler_t* com) 
{
    int len = 0;
    int index = com->index;
    rune* start = arena_get(com->utf8_file_content, com->index);
    while (true) {
        ADVANCE
        if (c == '"') {
            string_builder_t* string = stringb_new(len , start);
            TOKEN(len - 1, TOKEN_STRING_LITERAL)
            tok->index = com->index;
            tok->s_value = string;
            return;
        }
        len++;
        index++;
    }
}

static bool parse_hex_literal(compiler_t* com) 
{
    int next_whitespace_index = get_end_of_number(com);
    int result = 0;
    int index = 0;
    for (int i = next_whitespace_index - com->index; i >= 0; i--) {
        char c = *((rune*)arena_get(com->utf8_file_content, com->index + i));;
        if (c == '_') continue;
        if (c == '0') {index++; continue;}

        int digit = -1;
        if      (c >= '1' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else if (is_whitespace(c)) {
            MAKE_ERROR(false, com->line, com->col - 2, 2, "error: Hexadecimal number literal cannot be empty")
            TOKEN(next_whitespace_index - com->index, TOKEN_ERROR);
            return false;
        }
        else {
            int pos = ((next_whitespace_index - com->index) - i);
            MAKE_ERROR(false, com->line, com->col + (next_whitespace_index - com->index) - pos, 1, "error: Invalid character in Hex number literal")
            TOKEN(next_whitespace_index - com->index, TOKEN_ERROR);
            tok->i_value = result;
            com->col  += next_whitespace_index - com->index;
            com->index = next_whitespace_index + 1;
            return false;
        }
        result += digit * pow(16, index);
        index++;
    }
    TOKEN(next_whitespace_index - com->index + 1, TOKEN_I_NUMBER_LITERAL)
    tok->i_value = result;
    com->col  += next_whitespace_index - com->index;
    com->index = next_whitespace_index + 1;
    return false;
}

static bool parse_bin_literal(compiler_t* com) 
{
    int next_whitespace_index = get_end_of_number(com);
    int result = 0;
    int index = 0;
    for (int i = next_whitespace_index - com->index; i >= 0; i--) {
        char c = *((rune*)arena_get(com->utf8_file_content, com->index + i));;
        if (c == '_') continue;
        if (c == '0') {index++; continue;}

        int digit = -1;
        if      (c == '1' || c == '0') digit = c - '0';
        else if (is_whitespace(c)) {
            MAKE_ERROR(false, com->line, com->col - 2, 2, "error: Binary number literal cannot be empty")
            TOKEN(next_whitespace_index - com->index, TOKEN_ERROR);
            return false;
        }
        else {
            int pos = ((next_whitespace_index - com->index) - i);
            MAKE_ERROR(false, com->line, com->col + (next_whitespace_index - com->index) - pos, 1, "error: Invalid character in binary number literal")
            TOKEN(next_whitespace_index - com->index, TOKEN_ERROR);
            tok->i_value = result;
            com->col  += next_whitespace_index - com->index;
            com->index = next_whitespace_index + 1;
            return false;
        }
        result += digit * pow(2, index);
        index++;
    }
    TOKEN(next_whitespace_index - com->index + 1, TOKEN_I_NUMBER_LITERAL)
    tok->i_value = result;
    com->col  += next_whitespace_index - com->index;
    com->index = next_whitespace_index + 1;
    return false;
}

static bool parse_oct_literal(compiler_t* com) 
{
    int next_whitespace_index = get_end_of_number(com);
    int result = 0;
    int index = 0;
    for (int i = next_whitespace_index - com->index; i >= 0; i--) {
        char c = *((rune*)arena_get(com->utf8_file_content, com->index + i));;
        if (c == '_') continue;
        if (c == '0') {index++; continue;}

        int digit = -1;
        if      (c >= '1' && c <= '8') digit = c - '0';
        else if (is_whitespace(c)) {
            MAKE_ERROR(false, com->line, com->col - 2, 2, "error: Octal number literal cannot be empty")
            TOKEN(next_whitespace_index - com->index, TOKEN_ERROR);
            return false;
        }
        else {
            int pos = ((next_whitespace_index - com->index) - i);
            MAKE_ERROR(false, com->line, com->col + (next_whitespace_index - com->index) - pos, 1, "error: Invalid character in Octal number literal")
            TOKEN(next_whitespace_index - com->index, TOKEN_ERROR);
            tok->i_value = result;
            com->col  += next_whitespace_index - com->index;
            com->index = next_whitespace_index + 1;
            return false;
        }
        result += digit * pow(8, index);
        index++;
    }
    TOKEN(next_whitespace_index - com->index + 1, TOKEN_I_NUMBER_LITERAL)
    tok->i_value = result;
    com->col  += next_whitespace_index - com->index;
    com->index = next_whitespace_index + 1;
    return false;
}

static bool parse_number_literal(compiler_t* com) 
{
    char first_digit = advance(com);

    int index = 0;
    if (first_digit == '0') {
        char c = advance(com);
        if      (c == 'x') return parse_hex_literal(com);
        else if (c == 'b') return parse_bin_literal(com);
        else if (c == 'o') return parse_oct_literal(com);
        else if (!(c >= '0' && c <= '9')) {
            MAKE_ERROR(false, com->line, com->col - 1, 1, "error: invalid format specifier for int literal.")
            TOKEN(1, TOKEN_ERROR)
        }   
        com->index--;
        com->col--;
    }
    
    com->index--;
    com->col--;
    int next_whitespace_index = get_end_of_number(com);
    int result = 0;
    for (int i = next_whitespace_index - com->index; i >= 0; i--) {
        char c = *((rune*)arena_get(com->utf8_file_content, com->index + i));;
        if (c == '_') continue;
        if (c == '0') { index++; continue; }

        int digit = -1;
        if  (c >= '1' && c <= '9') digit = c - '0';
        else {
            break;
        }
        result += digit * pow(10, index);
        index++;
    }
    com->col++;
    TOKEN(next_whitespace_index - com->index + 1, TOKEN_I_NUMBER_LITERAL)
    tok->i_value = result;
    com->col  += next_whitespace_index - com->index ;
    com->index = next_whitespace_index + 1;

    int len = 0;
    return false;
}

// TODO: Parse floating point numbers
// TODO: Parse identifiers and keywords like false, true etc.
void lexer_lex_file(compiler_t* com)
{
    com->token_t_allocator = arena_new(sizeof(token_t), 5);

    while (com->index < len_arena(com->utf8_file_content)) {
        ADVANCE_OR_BREAK
        if (c == '\r') {
            com->index++;
            com->line++;
            com->col = 0;
            continue;
        }
        if (c == '\n') {
            com->index++;
            com->line++;
            com->col = 0;
            continue;
        }
        #pragma region single_ops
        if (c == '+') {
            TOKEN(1, TOKEN_PLUS)
            continue;
        }
        if (c == '-') {
            TOKEN(1, TOKEN_MINUS)
            continue;
        }
        if (c == '*') {
            TOKEN(1, TOKEN_MUL)
            continue;
        }
        if (c == '%') {
            TOKEN(1, TOKEN_MOD)
            continue;
        }
        if (c == ';') {
            TOKEN(1, TOKEN_SEMI)
            continue;
        }
        if (c == '(') {
            TOKEN(1, TOKEN_LPAREN)
            continue;
        }
        if (c == ')') {
            TOKEN(1, TOKEN_RPAREN)
            continue;
        }
        if (c == '^') {
            TOKEN(1, TOKEN_XOR)
            continue;
        }
        #pragma endregion single_ops
        #pragma region two_char_ops
        if (c == '/') {
            c = peek(com);
            if (c == '/') {
                token_t *tok = arena_alloc(com->token_t_allocator);
                tok->col = com->col;
                tok->line = com->line;
                tok->length = 2;
                tok->type = TOKEN_COMMENT;
                advance(com);
                bool eof = skip_until_newline(com);
                if (eof) break;
                continue;
            }
            token_t *tok = arena_alloc(com->token_t_allocator);
            tok->col = com->col;
            tok->line = com->line;
            tok->length = 1;
            tok->type = TOKEN_DIV;
            continue;
        }
        if (c == '=') {
            dToken('=', TOKEN_SEQ, TOKEN_EQ)
            continue;
        }
        if (c == '!') {
            dToken('=', TOKEN_NOT, TOKEN_NEQ)
            continue;
        }
        if (c == '<') {
            dToken('=', TOKEN_LT, TOKEN_LEQ)
            continue;
        }
        if (c == '>') {
            dToken('=', TOKEN_GT, TOKEN_GEQ)
            continue;
        }
        if (c == '&') {
            dToken('&', TOKEN_BAND, TOKEN_AND)
            continue;
        }
        if (c == '|') {
            dToken('|', TOKEN_BOR, TOKEN_OR);
            continue;
        }
        #pragma endregion two_char_ops
        if (c == '\"') {
            parse_string_literal(com);
            continue;
        }
        if (c >= '0' && c <= '9') {
            com->col--;
            com->index--;
            parse_number_literal(com);
            continue;
        }
    }
    TOKEN(1, TOKEN_EOF);
#ifdef PRINT_DEBUGS_
    for (int i = 0; i <= com->token_t_allocator->index; i++) {
        token_t* tok = arena_get(com->token_t_allocator, i);
        if (tok == null) break;
        const char* token_name = token_names[tok->type];
        printf("%d: type: %s => %d", i, token_name, tok->type == TOKEN_I_NUMBER_LITERAL ? tok->i_value : 0);
        if (tok->type == TOKEN_STRING_LITERAL) {
            printf(" / %ls\n", (rune*)stringb_to_cstring(tok->s_value));
        }
        else { printf("\n"); }
    }
#endif
}