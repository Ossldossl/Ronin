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

bool match(Parser* p, TokenKind expected) {
    if (peek(p)->kind == expected) {
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
        make_error(const_str("Imported file must be a valid .rn file"), path->loc); return null;
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

void parse_toplevel_stmt(Parser* p) {
    Token* ident = p->cur;
    Token* next = get_next(p);
    if (next->kind == TOKEN_COLON && peek(p)->kind == TOKEN_COLON) {
        advance(p);
        
    } else {
        make_error("Expected '::' after identifier in global scope", next->loc);
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