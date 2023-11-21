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
    index++;
    while (c != '\n' && c != '\x20' && c != '\x09' && c != '\r' && index < len_arena(com->utf8_file_content)) {
        c = *((rune*)arena_get(com->utf8_file_content, index));
        index++;
    }
    if (index == com->index) return com->index;
    return index - 1;
}

static bool is_whitespace(rune c) 
{
    return (c == '\x20' || c == '\x09' || c == '\v');
}

static int get_end_of_number(compiler_t* com) 
{
    int index = com->index;
    rune c = *((rune*)arena_get(com->utf8_file_content, index));
    while (!is_whitespace(c) && index < len_arena(com->utf8_file_content) - 1) {
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
    }
}

static token_t* match_identifier(compiler_t* com, int start, int length, const char* rest, string_builder_t* string, token_type_t token)
{
    if (length == string->length && memcmp(&string->content[start], rest, length - start)) {
        token_t *tok = arena_alloc(com->token_t_allocator);
        tok->col = com->col;
        tok->line = com->line;
        tok->length = length;
        tok->type = token;
        stringb_free(string);
        return tok;
    } else {
        TOKEN(string->length, TOKEN_IDENTIFIER);
        return tok;
    }
}

static token_t* choose_identifier_or_keyword(compiler_t* com, string_builder_t* string) 
{
    // TODO: Parse all keywords
    rune c = string->content[0];
    if (c == 'f') {
        c = string->content[1];
        if (c == 'a') return match_identifier(com, 2, 5, "lse", string, TOKEN_FALSE);
        else return match_identifier(com, 0, 2, "fn", string, TOKEN_fn);
    }
}

static void parse_identifier_or_keyword(compiler_t* com) 
{
    int len = 0;
    string_builder_t* string = null;
    rune* start = arena_get(com->utf8_file_content, com->index);
    while (true) {
        rune c = advance(com);
        if (c == 0 || is_whitespace(c) || c == '\x0D' || c == '\x0A') {
            com->index--;
            com->col--;
            string = stringb_new(len, start);
            break;
        }
        len++;
    }
    if (len == 0 || string == null) return;
    choose_identifier_or_keyword(com, string);
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
        char c = advance(com);
        if (c == '_') continue;
        if (c == '0') { index++; continue; }

        int digit = -1;
        if  (c >= '1' && c <= '9') digit = c - '0';
        else {
            com->index--;
            com->col--;
            break;
        }
        result *= 10;
        result += digit;
        index++;
    }

    TOKEN(next_whitespace_index - com->index + 1, TOKEN_I_NUMBER_LITERAL)
    tok->i_value = result;

    return false;
}

// TODO: Parse floating point numbers
// TODO: Parse identifiers and keywords like false, true etc.
void lexer_lex_file(compiler_t* com)
{
    com->token_t_allocator = arena_new(sizeof(token_t), 5);

    while (com->index < len_arena(com->utf8_file_content)) {
        ADVANCE_OR_BREAK
        if (is_whitespace(c)) {
            continue;
        }
        if (c == '\r') {
            com->index++;
            com->line++;
            com->col = 0;
            continue;
        }
        if (c == '\n') {
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
            dToken('>', TOKEN_MINUS, TOKEN_ARROW)
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
        com->index--;
        com->col--;
        parse_identifier_or_keyword(com);
        /*print_message(COLOR_RED, "This should not be reached!\n");
        token_t *tok = arena_alloc(com->token_t_allocator);
        tok->col = com->col;
        tok->line = com->line;
        tok->length = search_whitespace(com) - com->index;
        tok->type = TOKEN_ERROR;
        MAKE_ERROR(false, com->line, com->col - 1, tok->length + 1, "error: Invalid Token ??")
        com->index += tok->length;
        com->col += tok->length;*/
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