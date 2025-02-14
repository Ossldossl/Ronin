#include "parser.h"
#include "file.h"
#include <stdlib.h>
#include <string.h>

extern Arena arena;
extern Compiler compiler;

// DEBUG STUFF

[[noreturn]] void print_errors_and_exit(void) {
    u32 _count;
    for_array(&compiler.errors, Error) 
        // TODO: error printing
    }

    exit(-1);
}

// MAIN PARSER PART

// returns true on eof;
bool advance(Parser* p) {
    if (p->cur->kind == TOKEN_EOF) return true;
    p->cur_tok += 1;
    p->cur = array_get(p->tokens, p->cur_tok);
    return false;
}

Token* get_next(Parser* p) {
    advance(p);
    return p->cur;
}

Token* peek(Parser* p) {
    if (p->cur->kind == TOKEN_EOF) return p->cur;
    return array_get(p->tokens, p->cur_tok+1);
}

bool expect(Parser* p, TokenKind expected) {
    if (peek(p)->kind == expected) {
        advance(p);
        return true;
    }
    return false;
}

bool match(Parser* p, TokenKind expected) {
    if (p->cur->kind == expected) {
        advance(p);
        return true;
    }
    return false;
}

void parse_import(Parser* p, Str8 ident) {
    Token* import = p->cur;
    Token* path = get_next(p);
    advance(p);
    if (path->kind != TOKEN_STR_LIT) {
        make_error(const_str("Expected file path after import statement!"), path->loc);
        return;
    }
    u32 path_len;
    char* abs_path = path_to_absolute(str_to_cstr(&path->as._str), path->as._str.len, &path_len); 
    if (!file_exists(abs_path)) {
        Str8 ending = str_get_last_n(&path->as._str, 4);
        if (!str_cmp_c(&ending, ".rn")) {
            // maybe supplied a directory
            char* new_ending = arena_alloc(&arena, 7);
            // overwrite the previous null byte
            memcpy_s(new_ending, 8, "\\lib.rn", 8);
            path_len+=8;
            if (file_exists(abs_path)) {
                // find identifier for directory import
                ident = file_get_ident(abs_path, path_len);
                goto compiler_import_file_finalize;
            } else {
                make_error(const_str("Directory is not a valid lib or does not exist!"), path->loc); return;
            }
        } else {
            make_error(const_str("File or directory not found!"), path->loc);
            return;
        }
    }
    // validate ending
    char* ending = abs_path+path_len-3;
    if (strcmp(ending, ".rn") != 0) { 
        make_error(const_str("Imported file must be a valid .rn file"), path->loc);
    }
    
compiler_import_file_finalize:
    if (ident.len == 0) {
        // find ident
        ident = file_get_ident(abs_path, path_len);
    }

    Module* mod = map_get(&compiler.imported_files, abs_path, path_len);
    if (mod != null) {
        // file already imported
        map_set(&p->cur_mod->imports, abs_path, path_len, mod);
    } else {
        // parse file
        // save cd
        Str8 prev_dir = get_current_directory();
        // set new cd
        Str8 dir = get_dir_name(abs_path);
        bool ok = set_current_directory(dir);
        arena_free_last(&arena);

        // read file
        char* file_content;
        u64 file_size = read_file(abs_path, &file_content);
        u16 file_id = compiler.cur_file_id++;

        // parse file
        u32 prev_err_count = compiler.errors.used;
        Array toks = lexer_lex_str(make_str(file_content, file_size), file_id);
        Module* imported_mod = parse_tokens(toks);

        // report errors
        if (compiler.errors.used != prev_err_count) {
            print_errors_and_exit();
        }

        map_set(&compiler.imported_files, abs_path, path_len, imported_mod);
        map_set(&p->cur_mod->imports, abs_path, path_len, mod);

        // reset to old cd
        set_current_directory(prev_dir);
    }
    // TODO: make strs be zero terminated by default

    log_debug("Imported file: %s as %s", abs_path, str_to_cstr(&ident));
    arena_free_last(&arena);
}

Expr* parse_if(Parser* p) {

}

Expr* parse_match(Parser* p) {
    // TODO:
    log_fatal("parsing match is not implemented yet"); exit(-1);
}

Expr* parse_block(Parser* p) {
    log_fatal("parsing block is not implemented yet!"); exit(-1);
}

inline u8 postfix_binding_power(Token tok) 
{
    switch (tok.kind) {
        case TOKEN_NOT     : return 11;
        case TOKEN_LBRACKET: return 11;
        case TOKEN_LPAREN  : return 11;
             default       : return 0;
    }
}

inline u8 prefix_binding_power(Token tok) {
    switch (tok.kind) {
        case TOKEN_PLUS : return 9;
        case TOKEN_MINUS: return 9;
             default    : return 0;
    }
} 

inline u8 infix_binding_power(Token tok, u8* l_bp, u8* r_bp) {
    switch (tok.kind) {
        case TOKEN_LOR: {
            *r_bp = 2;
            *l_bp = 1;
            return BINARY_LOR;
        } break;

        case TOKEN_LAND: {
            *r_bp = 4;
            *l_bp = 3;
            return BINARY_LAND;
        } break;

        case TOKEN_BOR: {
            *r_bp = 6;
            *l_bp = 5;
            return BINARY_BOR;
        } break;

        case TOKEN_XOR: {
            *r_bp = 8;
            *l_bp = 7;
            return BINARY_XOR;
        } break;

        case TOKEN_BAND: {
            *r_bp = 10;
            *l_bp = 9;
            return BINARY_BAND;
        } break;

        case TOKEN_EQ: {
            *r_bp = 12;
            *l_bp = 11;
            return BINARY_EQ;
        }
        case TOKEN_NEQ: {
            *r_bp = 12;
            *l_bp = 11;
            return BINARY_NEQ;
        }

        case TOKEN_LEQ: {
            *r_bp = 14;
            *l_bp = 13;
            return BINARY_LEQ;
        } break;
        case TOKEN_GEQ: {
            *r_bp = 14;
            *l_bp = 13;
            return BINARY_GEQ;
        } break;
        case TOKEN_LT: {
            *r_bp = 14;
            *l_bp = 13;
            return BINARY_LT;
        } break;
        case TOKEN_GT: {
            *r_bp = 14;
            *l_bp = 13;
            return BINARY_GT;
        } break;
        
        case TOKEN_LSHIFT: {
            *r_bp = 16;
            *l_bp = 15;
            return BINARY_LSHIFT;
        } break;
        case TOKEN_RSHIFT: {
            *r_bp = 16;
            *l_bp = 15;
            return BINARY_RSHIFT;
        } break;
        
        case TOKEN_PLUS: {
            *r_bp = 18;
            *l_bp = 17;
            return BINARY_ADD;
        } break;
        case TOKEN_MINUS: {
            *r_bp = 18;
            *l_bp = 17;
            return BINARY_SUB;
        } break;

        case TOKEN_ASTERISK: {
            *r_bp = 20;
            *l_bp = 19;
            return BINARY_MUL;
        } break;
        case TOKEN_SLASH: {
            *r_bp = 20;
            *l_bp = 19;
            return BINARY_DIV;
        } break;
        case TOKEN_MODULO: {
            *r_bp = 20;
            *l_bp = 19;
            return BINARY_MOD;
        } break;

        case TOKEN_PERIOD: {
            *r_bp = 22;
            *l_bp = 21;
            return BINARY_MEMBER_ACCESS;
        } break;

        default: return BINARY_INVALID;
    }
}

Expr* parse_expr_bp(Parser* p, u8 min_bp) 
{
    if (p->cur->kind == TOKEN_IF) {
        return parse_if(p);
    } else if (p->cur->kind == TOKEN_MATCH) {
        return parse_match(p);
    } else if (p->cur->kind == TOKEN_DO) {
        // block
        return parse_block(p);
    }
    Expr* lhs = null;
    // unary expressions
    Token* last_tok = p->cur;
    if (match(p, TOKEN_BAND)) {
        lhs = arena_alloc(&arena, sizeof(Expr));
        lhs->kind = EXPR_UNARY;
        lhs->un.kind = UNARY_ADDRESS_OF;
        lhs->loc = last_tok->loc;
        lhs->un.rhs = parse_expr_bp(p, 0);
    } else if (match(p, TOKEN_NOT)) {
        lhs = arena_alloc(&arena, sizeof(Expr));
        lhs->kind = EXPR_UNARY;
        lhs->un.kind = UNARY_BNOT;
        lhs->loc = last_tok->loc;
        lhs->un.rhs = parse_expr_bp(p, 0);
    } else if (match(p, TOKEN_MINUS)) {
        lhs = arena_alloc(&arena, sizeof(Expr));
        lhs->kind = EXPR_UNARY;
        lhs->un.kind = UNARY_NEGATE;
        lhs->loc = last_tok->loc;
        lhs->un.rhs = parse_expr_bp(p, 0);
    } else if (match(p, TOKEN_PLUS)) {
        lhs = parse_expr_bp(p, 0);
    } else if (match(p, TOKEN_LPAREN)) {
        lhs = parse_expr_bp(p, 0);
        if (!match(p, TOKEN_RPAREN)) {
            make_errorh(const_str("Missing closing parenthesis here"), p->cur->loc, const_str("To close this one"), last_tok->loc);
        }
    }

    Expr* rhs = null;
    while (true) {
        Token op = *peek(p);
        if (op.kind == TOKEN_EOF) break;

        u8 l_bp = postfix_binding_power(op);
        // postfix expressions
        if (l_bp != 0) {
            if (l_bp < min_bp) break;
            advance(p);

            Expr* post = arena_alloc(&arena, sizeof(Expr));
            post->kind = EXPR_POST;
            post->loc = op.loc;
            if (op.kind == TOKEN_PLUS && peek(p)->kind == TOKEN_PLUS) {
                post->post.op_kind = POST_INC;
                advance(p);
            } else if (op.kind == TOKEN_MINUS && peek(p)->kind == TOKEN_MINUS) {
                post->post.op_kind = POST_DEC;
                advance(p);
            } else if (op.kind == TOKEN_LPAREN) {
                Array args = array_init(sizeof(Expr));
                while (p->cur->kind != TOKEN_RPAREN) {
                    Expr* arg = parse_expr_bp(p, 0);
                    Expr* slot = array_append(&args);
                    memcpy_s(slot, sizeof(Expr), arg, sizeof(Expr));
                    match(p, TOKEN_COMMA);
                } advance(p);
                post->post.op_kind = POST_FN_CALL;
            }
            post->post.lhs = lhs;
            lhs = post;
            continue;
        }

        l_bp = 0; u8 r_bp = 0;
        BinaryKind k = infix_binding_power(op, &l_bp, &r_bp);
        if (l_bp != 0) {
            if (l_bp <= min_bp) break;
            advance(p); // skip op
            rhs = parse_expr_bp(p, r_bp);
            Expr* bin_exp = arena_alloc(&arena, sizeof(Expr));
            bin_exp->kind = EXPR_BINARY;
            bin_exp->loc = op.loc;
            bin_exp->bin.lhs = lhs; bin_exp->bin.rhs = rhs;
            bin_exp->bin.kind = k;
            lhs = bin_exp; rhs = null;
            continue;
        }
        break;
    }
    return lhs;
}

void parse_toplevel_stmt(Parser* p) { 
    Token* ident = p->cur;
    Token* next = get_next(p);
    if (next->kind == TOKEN_COLON && peek(p)->kind == TOKEN_COLON) {
        advance(p);
        switch (p->cur->kind) {
            default: {
                parse_expr_bp(p, 0);
            } break;
            break;
        }
    } else {
        make_error(const_str("Expected '::' after identifier in global scope"), next->loc);
    }
}

Module* parse_tokens(Array tokens) 
{
    Parser parser = {0};
    parser.tokens = &tokens;
    parser.cur = array_get(&tokens, 0);
    
    while (true) {
        Token* tok = parser.cur;
        if (tok->kind == TOKEN_IDENT) {
            parse_toplevel_stmt(&parser);
        } else if (tok->kind == TOKEN_IMPORT) {
            parse_import(&parser, null_str);
        } else {
            make_error(const_str("unexpected token"), tok->loc);
        }
    }
    return parser.cur_mod;
}