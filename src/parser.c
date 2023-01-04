#include "include/misc.h"

void parser_parse_tokens(compiler_t* com) 
{
    for (int i = 0; i < com->token_t_allocator->index; i++) {
        token_t* tok = (token_t*)arena_get(com->token_t_allocator, i);
        printf("%d: %s => %d; at %d:%d with length: %d\n", i, token_names[tok->type], tok->type == TOKEN_I_NUMBER_LITERAL ? tok->i_value : -1, tok->line, tok->col, tok->length);
    }
}