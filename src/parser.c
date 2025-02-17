#include "parser.h"
#include "file.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Arena arena;
extern Compiler compiler;

extern const char* log_levels[];

TypeRef parse_type(Parser* p);
Expr* parse_block(Parser* p);

char* get_line(Str8 file_content, u32 line)
{
    int cur = 1;
    int i = 0;
    while (true) {
        char c = file_content.data[i];
        if (c == '\0' || i >= file_content.len) return null;
        
        if (cur == line) {
            char* start = &file_content.data[i];
            uint32_t len = 0;
            while (true) {
                c = file_content.data[i++];
                if (c == '\r' || c == '\n' ||c == '\0') {
                    break;
                }
                len++;
            }
            char* line = arena_alloc(&arena, len+1);
            memcpy_s(line, len+1, start, len);
            line[len] = '\0';
            return line;
        }

        if (c == '\r') { 
            cur++; i++;
        }
        if (c == '\n') {
            cur++;
        }
        i++;
    }
}

void print_code_line(Str8 file_content, u32 line_number)
{
    console_set_color(COLOR_GREY);
    printf("%d  ", line_number); 
    console_reset();
    char* line = get_line(file_content, line_number);
    if (line == null) {
        log_error("Line is null!");
        exit(-4);
        return;
    }
    printf("%s", line);
    arena_free_last(&arena);
}

void print_error_msg(Span loc, log_level_e level, char* fmt, ...)
{
    console_set_color(COLOR_GREY);
    console_print_time();

    console_set_color(level+1);
    console_set_bold();
    printf("%s", log_levels[level]);
    console_reset_bold();

    console_set_color(COLOR_GREY);
    Str8* str = (Str8*)array_get(&compiler.filenames, loc.file_id);
    printf("%s:%d:%d ", str_to_cstr(str), loc.line, loc.col);
    console_reset();

    va_list args; va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

void print_inline_msg(bool is_hint, Span loc, Str8 text) 
{
    printf("\n  "); 

    int digits_of_line_number = log10(loc.line)+1;
    for (int i = 0; i < loc.col + digits_of_line_number; i++) {
        printf(" ");
    }

    console_set_bold(); console_set_color(is_hint ? COLOR_CYAN : COLOR_RED);
    for (int i = 0; i < loc.len; i++) {
        printf("^");
    }

    printf(" -- %s", is_hint ? "Hint: " : "Error: ");
    printf("%s\n", text.data);
    console_reset();
}

[[noreturn]] void print_errors_and_exit(void) {
    u32 _count;
    u16 last_file = 0;
    Str8 cur_src = {0};
    for_array(&compiler.errors, Error) 
        if (e->err_loc.file_id != last_file) {
            Str8* result = array_get(&compiler.sources, e->err_loc.file_id);
            if (result == null) {
                log_fatal("File id %d not found!", e->err_loc.file_id);
                exit(-1);
            }
            cur_src = *result;
            last_file = e->err_loc.file_id;
        }

        print_error_msg(e->err_loc, e->is_warning ? LOG_WARN : LOG_ERROR, e->err_text.data);
        if (e->hint_text.len == 0) {
            print_code_line(cur_src, e->err_loc.line);
            print_inline_msg(false, e->err_loc, e->err_text);
            printf("\n"); continue;
        } 
        // else
        if (e->hint_loc.line == e->err_loc.line) {
            // hint and error are on the same line
            print_code_line(cur_src, e->err_loc.line);
            bool hint_first = e->hint_loc.col > e->err_loc.col;
            if (hint_first) { print_inline_msg(true, e->hint_loc, e->hint_text); }
            print_inline_msg(false, e->err_loc, e->err_text);
            if (!hint_first) {
                print_inline_msg(true, e->hint_loc, e->hint_text);
            }
            printf("\n");
        } else {
            print_code_line(cur_src, e->err_loc.line);
            print_inline_msg(false, e->err_loc, e->err_text);
            printf("\n");
            print_code_line(cur_src, e->hint_loc.line);
            print_inline_msg(true, e->hint_loc, e->hint_text);
            printf("\n");
        }
    }

    exit(-1);
}

// MAIN PARSER PART

Scope* scope_push(Parser* p) {
    Scope* result = arena_alloc(&arena, sizeof(Scope));
    result->parent = p->cur_scope;
    result->syms = (Map){0};
    p->cur_scope = result;
    return result;
}

void scope_pop(Parser* p) {
    Scope* prev = p->cur_scope->parent;
    p->cur_scope = prev;
}

// sets variable in the current scope
void scope_seth(Parser* p, u64 hash, void* value) {
    map_seth(&p->cur_scope->syms, hash, value);
}

void scope_set(Parser* p, char* key, u32 len, void* value) {
    map_set(&p->cur_scope->syms, key, len, value);
}

void scope_sets(Parser* p, Str8 key, void* value) {
    map_sets(&p->cur_scope->syms, key, value);
}

void scope_symbol_sets(Parser* p, Str8 key, void* value, SymKind kind){ 
    Symbol* sym = arena_alloc(&arena, sizeof(Symbol));
    sym->name = key;
    sym->fn_ = (Fn*)value; // don't care to what this value is assigned to
    scope_sets(p, key, sym);
}

void* scope_geth(Parser* p, u64 hash) {
    Scope* cur = p->cur_scope;
    do {
        void* result = map_geth(&cur->syms, hash);
        if (result) return result;
        cur = cur->parent;
    } while (cur != null);
    return null;
}

void* scope_get(Parser* p, char* key, u32 len) {
    Scope* cur = p->cur_scope;
    do {
        void* result = map_get(&cur->syms, key, len);
        if (result) return result;
        cur = cur->parent;
    } while (cur != null);
    return null;
}

void* scope_gets(Parser* p, Str8 key) {
    Scope* cur = p->cur_scope;
    do {
        void* result = map_gets(&cur->syms, key);
        if (result) return result;
        cur = cur->parent;
    } while (cur != null);
    return null;
}

// returns true on eof;
static bool advance(Parser* p) {
    if (p->cur->kind == TOKEN_EOF) return true;
    p->cur_tok += 1;
    p->cur = array_get(p->tokens, p->cur_tok);
    return false;
}

static Token* get_next(Parser* p) {
    advance(p);
    return p->cur;
}

static Token* peek(Parser* p) {
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
        Str8* src = array_append(&compiler.sources);
        src->len = file_size; src->data = file_content;
        Str8* file_name = array_append(&compiler.filenames);
        file_name->len = path_len; file_name->data = abs_path;

        // parse file
        u32 prev_err_count = compiler.errors.used;
        Array toks = lexer_lex_str(make_str(file_content, file_size), compiler.cur_file_id);
        Module* imported_mod = parse_tokens(toks);
        compiler.cur_file_id += 1;
        
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

Expr* parse_expr_bp(Parser* p, u8 min_bp);

Expr* parse_if(Parser* p) {
    Token* if_tok = p->cur;
    advance(p);
    Expr* if_expr = arena_alloc(&arena, sizeof(Expr));
    if_expr->kind = EXPR_IF;
    if_expr->loc = if_tok->loc;

    ExprIf* eif = &if_expr->if_expr;
    eif->condition = parse_expr_bp(p, 0);
    eif->body = parse_expr_bp(p, 0);
    if (match(p, TOKEN_ELSE)) {
        eif->alternative = parse_expr_bp(p, 0);
    } else {
        eif->alternative = null;
    }
    if (!match(p, TOKEN_END)) {
        make_error(const_str("Expected 'end' here"), p->cur->loc);
    }
    return if_expr;
}

Expr* parse_match(Parser* p) {
    // TODO:
    log_fatal("parsing match is not implemented yet"); exit(-1);
}

Stmt* parse_for(Parser* p) {

}

void recover_until_semicolon_or_end(Parser* p) {
    while (p->cur->kind != TOKEN_SEMICOLON && p->cur->kind != TOKEN_END) {
        advance(p);
    }
    advance(p);
}

void recover_until_semicolon(Parser* p) {
    while (p->cur->kind != TOKEN_SEMICOLON) {
        advance(p);
    }
    advance(p);
}

bool expr_is_const(Parser* p, Expr* ex) {
    // TODO:
    log_fatal("Checking if an expr is const is not implemented yet!");
    return true;
}

Stmt* parse_stmt(Parser* p) 
{
    Token* ident = p->cur;
    if (match(p, TOKEN_IDENT)) {
        Token* colon = p->cur;
        if (match(p, TOKEN_COLON)) {
            Token* assign_or_colon = p->cur;

            Stmt* s = arena_alloc(&arena, sizeof(Stmt));
            s->type = STMT_LET;
            s->loc = colon->loc;
            if (assign_or_colon->loc.col > s->loc.col) {
                s->loc.len = assign_or_colon->loc.col - s->loc.col;
            }
            
            if (match(p, TOKEN_ASSIGN)) {
                s->let_stmt.initializer = parse_expr_bp(p, 0);
                s->let_stmt.var->name = ident->as._str;
                s->let_stmt.var->type.type = null;
            } else if (match(p, TOKEN_COLON)) {
                // TODO: constant assignment
                s->let_stmt.initializer = parse_expr_bp(p, 0);
                if (!expr_is_const(p, s->let_stmt.initializer)) {
                    make_error(const_str("Expression for constant has to be evaluable at compile time"), s->let_stmt.initializer->loc);
                    recover_until_semicolon(p);
                }
            } else {
                // parse type
                s->let_stmt.var->type = parse_type(p);
                if (match(p, TOKEN_ASSIGN)) {
                    s->let_stmt.initializer = parse_expr_bp(p, 0);
                    s->let_stmt.var->name = ident->as._str;
                } else if (match(p, TOKEN_COLON)) {
                    // TODO: constant assignment
                    s->let_stmt.initializer = parse_expr_bp(p, 0);
                    if (!expr_is_const(p, s->let_stmt.initializer)) {
                        make_error(const_str("Expression for constant has to be evaluable at compile time"), s->let_stmt.initializer->loc);
                        recover_until_semicolon(p);
                    }
                }
            }
            match(p, TOKEN_SEMICOLON);
            return s;
        } else if (match(p, TOKEN_ASSIGN)) {
            Stmt* s = arena_alloc(&arena, sizeof(Stmt));
            s->type = STMT_ASSIGN;
            s->loc = ident->loc;
            s->assign_stmt.name = ident->as._str;
            s->assign_stmt.rhs = parse_expr_bp(p, 0);
            match(p, TOKEN_SEMICOLON);
            return s;
        }
        
        // parse expr
        Stmt* s = arena_alloc(&arena, sizeof(Stmt));
        s->type = STMT_EXPR;
        s->expr = parse_expr_bp(p, 0);
        s->loc = s->expr->loc;
        match(p, TOKEN_SEMICOLON);
        return s;
    } 
    else if (p->cur->kind == TOKEN_FOR) {
        log_fatal("Parsing for is not implemented yet!");
        exit(-1);
    } else if (p->cur->kind == TOKEN_WHILE) {
        Stmt* s = arena_alloc(&arena, sizeof(Stmt));
        s->type = STMT_WHILE_LOOP; s->loc = p->cur->loc;
        advance(p); // skip while
        s->while_loop.condition = parse_expr_bp(p, 0);
        if (!match(p, TOKEN_DO)) {
            make_error(const_str("Expected \"do\" after here"), p->cur->loc);
            recover_until_semicolon_or_end(p);
        }
        s->while_loop.body = &parse_block(p)->block; // parse block consumes 'end' 
        return s;
    } else if (p->cur->kind == TOKEN_RETURN) {
        Stmt* s = arena_alloc(&arena, sizeof(Stmt));
        s->type = STMT_RETURN; s->loc = p->cur->loc;
        advance(p); // skip return
        s->expr = parse_expr_bp(p, 0);
        match(p, TOKEN_SEMICOLON);
        return s;
    } else if (p->cur->kind == TOKEN_YIELD) {
        Stmt* s = arena_alloc(&arena, sizeof(Stmt));
        s->type = STMT_YIELD; s->loc = p->cur->loc;
        advance(p); // skip return
        s->expr = parse_expr_bp(p, 0);
        match(p, TOKEN_SEMICOLON);
        return s;
    }

    Expr* expr = parse_expr_bp(p, 0);
    if (expr == null) return null;

    Stmt* s = arena_alloc(&arena, sizeof(Stmt));
    s->type = STMT_EXPR;
    s->expr = expr;
    s->loc = s->expr->loc;
    match(p, TOKEN_SEMICOLON);
    return s;
}

Expr* parse_block(Parser* p) {
    Token* do_tok = null;
    if (p->cur->kind == TOKEN_DO) {
        do_tok = p->cur;
        advance(p);
    }
    Token* start = p->cur;

    Expr* block_expr = arena_alloc(&arena, sizeof(Expr));
    block_expr->kind = EXPR_BLOCK;
    block_expr->block.scope = scope_push(p);

    block_expr->block.stmts = array_init(sizeof(Stmt));
    while (p->cur->kind != TOKEN_END) {
        Stmt* s = parse_stmt(p);
        if (s) {
            Stmt** stmt_slot = array_append(&block_expr->block.stmts);
            *stmt_slot = s;
        }
    }

    Token* end_token = p->cur;
    if (!match(p, TOKEN_END)) {
        make_errorh(const_str("Expected \"end\" here"), p->cur->loc, const_str("To close the block here"), do_tok->loc);
    }
    block_expr->loc = do_tok->loc;
    return block_expr;
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
    if (p->cur->kind == TOKEN_END) return null;

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

    if (lhs == null) {
        // parse literal 
        switch (p->cur->kind) {
            case TOKEN_INT_LIT: {
                lhs = arena_alloc(&arena, sizeof(Expr));
                lhs->kind = EXPR_POST;
                lhs->loc = p->cur->loc;
                lhs->post.op_kind = POST_NONE;
                lhs->post.val_kind = POST_INT;
                lhs->post.lhs = null;
                lhs->post.value._int = p->cur->as._int;
                advance(p);
            } break;
            case TOKEN_FLOAT_LIT: {
                lhs = arena_alloc(&arena, sizeof(Expr));
                lhs->kind = EXPR_POST;
                lhs->loc = p->cur->loc;
                lhs->post.op_kind = POST_NONE;
                lhs->post.val_kind = POST_FLOAT;
                lhs->post.lhs = null;
                lhs->post.value._double = p->cur->as._double;
                advance(p);
            } break;
            case TOKEN_TRUE: {
                lhs = arena_alloc(&arena, sizeof(Expr));
                lhs->kind = EXPR_POST;
                lhs->loc = p->cur->loc;
                lhs->post.op_kind = POST_NONE;
                lhs->post.val_kind = POST_TRUE;
                lhs->post.lhs = null;
                lhs->post.value._bool = true;
                advance(p);
            } break;
            case TOKEN_FALSE: {
                lhs = arena_alloc(&arena, sizeof(Expr));
                lhs->kind = EXPR_POST;
                lhs->loc = p->cur->loc;
                lhs->post.op_kind = POST_NONE;
                lhs->post.val_kind = POST_FALSE;
                lhs->post.lhs = null;
                lhs->post.value._bool = false;
                advance(p);
            } break;
            case TOKEN_STR_LIT: {
                lhs = arena_alloc(&arena, sizeof(Expr));
                lhs->kind = EXPR_POST;
                lhs->loc = p->cur->loc;
                lhs->post.op_kind = POST_NONE;
                lhs->post.val_kind = POST_STR;
                lhs->post.lhs = null;
                lhs->post.value._str = p->cur->as._str;
                advance(p);
            } break;
            default: {
                make_error(const_str("Unexpected token"), p->cur->loc);
                return null;
            }   
        }
    }

    Expr* rhs = null;
    while (true) {
        Token op = *p->cur;
        if (op.kind == TOKEN_EOF || op.kind == TOKEN_SEMICOLON) break;

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

void parse_struct(Parser* p, Token* ident, bool is_generic, ArrayOf(GenericParam) generic_over) {
    // TODO STRUCT
}

void parse_enum(Parser* p, Token* ident, bool is_generic, ArrayOf(GenericParam) generic_over) {
    // TODO ENUM
} 

void parse_union(Parser* p, Token* ident, bool is_generic, ArrayOf(GenericParam) generic_over) {
    // TODO UNION
}

void parse_trait(Parser* p, Token* ident, bool is_generic, ArrayOf(GenericParam) generic_over) {
    // TODO TRAIT
}

TypeRef parse_type(Parser* p) {
    TypeRef result = {0};
    // TODOOOOOOOOO: syntax for references
    if (match(p, TOKEN_OWNED)) {
        result.is_owned = true;
    }

    TypeRef* cur = &result;
    while (match(p, TOKEN_BAND)) {
        cur->is_ptr = true; 
        cur->ptr = arena_alloc(&arena, sizeof(TypeRef));
        *cur->ptr = (TypeRef){0};
        cur = cur->ptr;
    }

    if (p->cur->kind != TOKEN_IDENT) {
        make_error(const_str("Expected identifier for type"), p->cur->loc);
        return result;
    }
    Symbol* type = scope_gets(p, p->cur->as._str);
    if (type == null) {
        make_error(const_str("Unknown type"), p->cur->loc);
        return result;
    }
    if (type->kind == SYM_TRAIT) {
        make_error(const_str("Expected a type, not a trait. Use generics with trait bounds instead"), p->cur->loc);
        return result;
    } else if (type->kind == SYM_EXPR || type->kind == SYM_FN) {
        make_error(const_str("Expected a type here. This is not a type"), p->cur->loc);
        return result;
    }
    
    if (cur->is_ptr) {
        cur = arena_alloc(&arena, sizeof(TypeRef));
        cur->is_ptr = false; cur->type = type->type_;
    }
    return result;
}

void parse_fn(Parser* p, Token* ident, bool is_generic, ArrayOf(GenericParam) generic_over) {
    advance(p);
    Fn* fn = arena_alloc(&arena, sizeof(Fn));
    fn->args = array_init(sizeof(Field));
    fn->is_foreign = fn->is_inline = false;
    fn->loc = ident->loc;
    fn->name = ident->as._str;
    
    scope_symbol_sets(p, fn->name, fn, SYM_FN);

    // parse args
    if (!match(p, TOKEN_LPAREN)) {
        make_error(const_str("Unexpected token"), p->cur->loc);
        return; // TODO: recover?
    }
    while (p->cur->kind != TOKEN_RPAREN) {
        Field* arg = array_append(&fn->args);
        Token* ident = p->cur;
        if (!match(p, TOKEN_IDENT)) {
            make_error(const_str("Expected identifier as function argument"), p->cur->loc);
            return;
        }
        if (!match(p, TOKEN_COLON)) {
            // TODO: allow multiple idents per type, like fn(arg1, arg2: i32)
            make_error(const_str("Expected type after argument identifier"), p->cur->loc);
            return; 
        }
        arg->type = parse_type(p);
        arg->name = ident->as._str;
        match(p, TOKEN_COMMA);
    }
    if (match(p, TOKEN_ARROW)) {
        // parse return type
        fn->return_type = parse_type(p);
    }
    
    // parse statements
    fn->body = array_init(sizeof(Stmt));
    fn->scope = scope_push(p);
    while (p->cur->kind != TOKEN_END) {
        Stmt* s = parse_stmt(p);
        if (s) {
            Stmt** stmt_slot = array_append(&fn->body);
            *stmt_slot = s;
        }
    }
    if (!match(p, TOKEN_END)) {
        make_error(const_str("Expected \"end\" here"), p->cur->loc);
    }
    scope_pop(p);
}

void parse_toplevel_stmt(Parser* p) { 
    Token* ident = p->cur;
    Token* next = get_next(p);
    
    Array generic_over = {0};
    bool is_generic = false;
    if (match(p, TOKEN_LT)) {
        // TODO: parse generics
        is_generic = true;
        generic_over = array_init(sizeof(GenericParam));
    }

    if (next->kind == TOKEN_COLON && peek(p)->kind == TOKEN_COLON) {
        advance(p); advance(p);
        switch (p->cur->kind) {
            case TOKEN_STRUCT: {
                return parse_struct(p, ident, is_generic, generic_over);
            } break;
            case TOKEN_ENUM: {
                return parse_enum(p, ident, is_generic, generic_over);
            } break;
            case TOKEN_TRAIT: {
                return parse_trait(p, ident, is_generic, generic_over);
            } break;
            case TOKEN_FN: {
                return parse_fn(p, ident, is_generic, generic_over);
            } break;
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
        } else if (tok->kind == TOKEN_EOF) {
            break;
        } else {
            make_error(const_str("unexpected token"), tok->loc);
            advance(&parser);
        }
    }
    return parser.cur_mod;
}