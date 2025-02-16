#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    compiler.filenames = array_init(sizeof(Str8));
    compiler.sources = array_init(sizeof(Str8));

    if (argc < 2) {
        printf("Usage: ronin <file>");
        exit(-1);
    }
    char* file_name = argv[1];

    Str8 dir = get_dir_name(file_name);
    bool ok = set_current_directory(dir);
    arena_free_last(&arena);

    Str8 input;
    input.len = read_file(file_name, &input.data);

    Str8* dummy = array_append(&compiler.sources); dummy->len = 0; // so that file ids can start at 1
    dummy = array_append(&compiler.filenames); dummy->len = 0;
    compiler.cur_file_id = 1;
    
    Str8* src = array_append(&compiler.sources);
    memcpy_s(src, sizeof(Str8), &input, sizeof(Str8));
    Str8* fn_str = array_append(&compiler.filenames);
    fn_str->len = strlen(file_name); fn_str->data = file_name;
    compiler.cur_file_id++;
    
    bool running = true;
    Array toks = lexer_lex_str(input, 1);
    if (compiler.errors.used != 0) {
        // has errors
        print_errors_and_exit();
    }
    Module* ast = parse_tokens(toks);
    if (compiler.errors.used != 0) {
        // has errors
        print_errors_and_exit();
    }
    printf("\n");
}