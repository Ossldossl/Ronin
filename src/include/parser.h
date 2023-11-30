#pragma once
#include "lexer.h"

// TODO: PARSER

typedef struct {
    array_t* import_paths; // todo
} ast_t;

void parser_parse_tokens(token_t* tokens, uint32_t token_count);