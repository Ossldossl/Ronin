#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "misc.h"
#include "console.h"
#include "arena.h"
#include "str.h"
#include "array.h"
#include "map.h"
#include "file.h"

Compiler compiler;
Arena arena;

Str8 read_line(void) {
    char* line = arena_alloc(&arena, 100);
    u32 cap = 100; u32 len = 0;
    char* cur = line;
    
    while (true) {
        char c = fgetc(stdin);
        if (c == EOF) break;
        if (--cap == 0) {
            arena_alloc(&arena, 100);
            cap = 100;
        }
        *cur = c; cur++;
        if (c == '\n') break;
    }
    return make_str(line, (u64)cur - (u64)line);
}


int main(int argc, char** argv) {
    init_console();
    arena = make_arena();
    compiler.errors = array_init(sizeof(Error));
    compiler.imported_files = (Map){0};
    compiler.cur_file_id = 1;

    /*
    Str8 dir = get_dir_name(file_name);
    bool ok = set_cur_dir(dir);
    arena_free_last(&arena);*/
    
    bool running = true;
    while (running) {
        printf("> ");
        Str8 input = read_line();
        Array toks = lexer_lex_str(input, 0);
        Module* ast = parse_tokens(toks);
        printf("\n");
    }
}