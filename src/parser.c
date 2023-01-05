#include "include/misc.h"

#define ROOT_NODE ((node_t*)arena_get(com->node_allocator, 0))

token_t* consume(compiler_t* com) 
{
    if (com->index == com->token_t_allocator->index + 1) {
        return null;
    }
    token_t* result = arena_get(com->token_t_allocator, com->index);
    com->index++;
    return result;
}

token_t* peek(compiler_t* com) 
{
    if (com->index == com->token_t_allocator->index + 1) {
        return null;
    }
    token_t* result = arena_get(com->token_t_allocator, com->index);
    return result;
}

token_t* previous(compiler_t* com) 
{
    token_t* result = arena_get(com->token_t_allocator, com->index - 1);
    return result;    
}

bool match(compiler_t* com, token_type_t expected) 
{
    token_t* tok = peek(com);
    if (tok == null || tok->type != expected) return false;
    com->index++;
    return true;
}

node_t* make_bin_node(compiler_t* com, bin_expr_type_t type, node_t* lhs, node_t* rhs, int line, int col, int length) 
{
    node_t* result = arena_alloc(com->node_allocator);
    result->type              = NODE_BINARY_EXPR;
    result->bin_expr.type   = type;
    result->bin_expr.lhs    = lhs;
    result->bin_expr.rhs    = rhs;
    result->bin_expr.line   = line;
    result->bin_expr.col    = col;
    result->bin_expr.length = length;
    return result;
}

node_t* make_unary_node(compiler_t* com, un_expr_type_t type, node_t* rhs, int line, int col, int length) 
{
    node_t* result = arena_alloc(com->node_allocator);
    result->type            = NODE_UNARY_EXPR;
    result->un_expr.type   = type;
    result->un_expr.rhs    = rhs;
    result->un_expr.line   = line;
    result->un_expr.col    = col;
    result->un_expr.length = length;
    return result;
}

node_t* make_value_node(compiler_t* com, int i_value) 
{
    node_t* result       = arena_alloc(com->node_allocator);
    result->type         = NODE_I_NUMBER_LITERAL;
    result->i_number_lit = i_value;
    return result;
}

node_t* parser_parse_expr(compiler_t* com);

node_t* parser_parse_primary(compiler_t* com) 
{
    token_t* tok = consume(com);
    if (tok == null) return null;
    if (tok->type == TOKEN_I_NUMBER_LITERAL)
    {
        return make_value_node(com, tok->i_value);
    }
    if (tok->type == TOKEN_LPAREN) {
        node_t* expr = parser_parse_expr(com);
        if (!match(com, TOKEN_RPAREN))
        {
            token_t* last_tok = previous(com);
            MAKE_ERROR(false, tok->line, tok->col - 1, 1, "error: This parenthesis is not closed anywere!");
            return null;
        }
        return expr;
    }
    return null;
}

node_t* parser_parse_unary(compiler_t* com) 
{
    node_t* rhs = null;
    if (match(com, TOKEN_NOT))
    {
        token_t* op_tok = previous(com);
        rhs = parser_parse_unary(com);
        return make_unary_node(com, UN_NOT, rhs, op_tok->line, op_tok->col, op_tok->length);
    }
    return parser_parse_primary(com);
}

node_t* parser_parse_factor(compiler_t* com)
{
    node_t*  lhs    = parser_parse_unary(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (true) {
        token_t* tok = peek(com);
        if (tok == null) return lhs;
        bin_expr_type_t op = 0;
        if (tok->type == TOKEN_MUL) op = BIN_MUL;
        else if (tok->type == TOKEN_DIV) op = BIN_DIV;
        else break;

        consume(com);
        op_tok = previous(com);
        rhs    = parser_parse_unary(com);
        lhs    = make_bin_node(com, op, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_term(compiler_t* com)
{
    node_t*  lhs    = parser_parse_factor(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (true) {
        token_t* tok = peek(com);
        if (tok == null) return lhs;
        bin_expr_type_t op = 0;
        if (tok->type == TOKEN_PLUS) op = BIN_ADD;
        else if (tok->type == TOKEN_MINUS) op = BIN_MINUS;
        else break;

        consume(com);
        op_tok = previous(com);
        rhs    = parser_parse_factor(com);
        lhs    = make_bin_node(com, op, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_equality(compiler_t* com) 
{
    node_t*  lhs    = parser_parse_term(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (true) {
        token_t* tok = peek(com);
        if (tok == null) return lhs;
        bin_expr_type_t op = 0;
        switch (tok->type){
            case TOKEN_LT:  { op = BIN_LT; break; }
            case TOKEN_GT:  { op = BIN_GT; break; }
            case TOKEN_EQ:  { op = BIN_EQ; break; }
            case TOKEN_NEQ: { op = BIN_NEQ; break; }
            case TOKEN_LEQ: { op = BIN_LEQ; break; }
            case TOKEN_GEQ: { op = BIN_GEQ; break; }
            default: return lhs;
        }
        op_tok = previous(com);
        consume(com);
        rhs    = parser_parse_term(com);
        lhs    = make_bin_node(com, op, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_band(compiler_t* com) 
{
    node_t*  lhs    = parser_parse_equality(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (match(com, TOKEN_BAND)) {
        op_tok = previous(com);
        rhs    = parser_parse_equality(com);
        lhs    = make_bin_node(com, BIN_BAND, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_bxor(compiler_t* com) 
{
    node_t*  lhs    = parser_parse_band(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (match(com, TOKEN_XOR)) {
        op_tok = previous(com);
        rhs    = parser_parse_band(com);
        lhs    = make_bin_node(com, BIN_XOR, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_bor(compiler_t* com) 
{
    node_t*  lhs    = parser_parse_bxor(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (match(com, TOKEN_BOR)) {
        op_tok = previous(com);
        rhs    = parser_parse_bxor(com);
        lhs    = make_bin_node(com, BIN_BOR, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_and(compiler_t* com) 
{
    node_t*  lhs    = parser_parse_bor(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (match(com, TOKEN_AND)) {
        op_tok = previous(com);
        rhs    = parser_parse_bor(com);
        lhs    = make_bin_node(com, BIN_AND, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_or(compiler_t* com) 
{
    node_t*  lhs    = parser_parse_and(com);
    node_t*  rhs    = null;
    token_t* op_tok = null;
    while (match(com, TOKEN_OR)) {
        op_tok = previous(com);
        rhs    = parser_parse_and(com);
        lhs    = make_bin_node(com, BIN_OR, lhs, rhs, op_tok->line, op_tok->col, 1);
    }
    return lhs;
}

node_t* parser_parse_expr(compiler_t* com) 
{
    node_t* result = parser_parse_or(com);
    return result;
}

void parser_parse_tokens(compiler_t* com) 
{
    com->index = 0;
    com->node_allocator = arena_new(sizeof(node_t), 1);

    node_t* root = arena_alloc(com->node_allocator);
    root->type   = NODE_ROOT;
    root->stmts  = vector_new();


    for (int i = 0; i < len_arena(com->utf8_file_content); i++) {
        token_t* tok = consume(com);
        if (tok == null) return;    
        switch (tok->type) 
        {
            case TOKEN_I_NUMBER_LITERAL: {
                // for now its awkward but when I add statements it'll get better
                com->index--;
                node_t* node = parser_parse_expr(com); 
                vector_push(ROOT_NODE->stmts, node);
                continue;
            }

            case TOKEN_EOF: { return; }
            default: {
                print_message(COLOR_RED, "Fatal error: Parsing of token of type %s is not implemented yet!", token_names[tok->type]); panic("\n"); 
            }
        }
    }
}