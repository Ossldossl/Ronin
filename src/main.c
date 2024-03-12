#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>
#include <time.h>

#include "include/console.h"
#include "include/allocators.h"
#include "include/array.h"
#include "include/map.h"

#include "include/lexer.h"
#include "include/parser.h"

#define HAS_ORIGINAL_ERRORS_ARRAY
#include "include/misc.h"
#undef HAS_ORIGINAL_ERRORS_ARRAY

#define null NULL

arena_t arena;
array_t errors;
array_t file_names;

void print_usage(void)
{
    console_set_bold();
    printf("Ronin Compiler\n");
    console_reset_bold(); console_set_underline(); console_set_bold();
    printf("USAGE:\n");
    console_reset_bold(); console_reset_underline();
    printf("    ronin <command> [arguments] <options>\n");
    console_set_underline(); console_set_bold();
    printf("Commands:\n");
    console_reset_underline();
    printf("    build"); console_reset_bold(); console_set_color(COLOR_GREY); 
    printf("    compile ronin file as an executable\n");
    console_reset(); console_set_bold();
    printf("    run  ");
    console_reset_bold(); console_set_color(COLOR_GREY);
    printf("    same as 'build', but also runs the compiled executable\n");
    console_reset();
    console_set_underline(); console_set_bold();
    printf("Options:\n");
    console_reset_bold(); console_reset_underline();
    printf("    --print-tokens: Prints all tokens in the first file\n");
    printf("    --print-ast: Prints the ast of the first file\n");
    console_reset();
    return;
}

HANDLE get_file_handle(const char* file_name, bool write) 
{
	HANDLE hFile;
	hFile = CreateFile(file_name,
		write ? GENERIC_WRITE : GENERIC_READ,
		write ? 0 : FILE_SHARE_READ,
		null,
		write ? CREATE_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL ,
		null);

    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DWORD err = GetLastError();
        if (err == 2) {
            log_fatal("Datei %s wurde nicht gefunden", file_name);
            exit(-1);
        } 
        log_fatal("Fehler beim Öffnen der Datei, %lu", err);
        exit(-1);
    }

    return hFile;
}

size_t open_file(const char* file_name, char** file_content)
{
    HANDLE hFile = get_file_handle(file_name, false);
    LARGE_INTEGER file_size_li;
    if (GetFileSizeEx(hFile, &file_size_li) == 0) {
        log_fatal("Fehler beim Abfragen der Größe der Datei, %lu", GetLastError());
        exit(-2);
    }
    size_t file_size = file_size_li.QuadPart;

    char* buf = arena_alloc(&arena, file_size+1);
    DWORD read = 0;
    if (ReadFile(hFile, buf, file_size, &read, null) == 0 || read == 0) {
        log_fatal("ERROR: Fehler beim Lesen der Datei, %lu", GetLastError());
        exit(-2);
    }
    CloseHandle(hFile);
    buf[file_size] = '\0';
    *file_content = buf;
    return file_size;
}

char* get_line(char* file_content, int line)
{
    int cur = 1;
    int i = 0;
    while (true) {
        char c = file_content[i];
        if (c == '\0') return null;
        
        if (cur == line) {
            char* start = &file_content[i];
            uint32_t len = 0;
            while (true) {
                c = file_content[i++];
                if (c == '\r' || c == '\n') {
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

void print_code_line(char* file_content, uint32_t line_number)
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

static int count_digits(uint32_t i)
{
    if (i == 0) return 1;
    int digits = 0;
    while (i != 0) {
        i /= 10;
        digits++;
    }
    return digits;
}

void print_err_msg(bool is_hint, span_t loc, char* msg)
{
    printf("\n  "); 

    int digits_of_line_number = count_digits(loc.line);
    for (int i = 0; i < loc.col-1 + digits_of_line_number; i++) {
        printf(" ");
    }

    console_set_bold(); console_set_color(is_hint ? COLOR_CYAN : COLOR_RED);
    for (int i = 0; i < loc.len; i++) {
        printf("^");
    }

    printf(" -- %s", is_hint ? "Hint: " : "Error: ");
    printf("%s", msg);
    console_reset();
}

void print_errors(char* file_content, int file_size)
{
    u32 _count;
    for_array(&errors, error_t) 
        bool print_extra_hint_line = false;
        c_msg(e->error_loc, e->lvl, " %s", e->error);

        if (e->hint == null) {
            print_code_line(file_content, e->error_loc.line);
            print_err_msg(false, e->error_loc, e->error);
            printf("\n");
            continue;
        } // else
        if (e->hint_loc.line == e->error_loc.line) {
            print_code_line(file_content, e->hint_loc.line);
            bool hint_first = e->hint_loc.col > e->error_loc.col;
            if (hint_first) { print_err_msg(true, e->hint_loc, e->hint); }
            print_err_msg(false, e->error_loc, e->error);
            if (!hint_first) {
                print_err_msg(true, e->hint_loc, e->hint);
            }
            printf("\n");
        } else {
            print_code_line(file_content, e->error_loc.line);
            print_err_msg(false, e->error_loc, e->error);
            printf("\n");
            print_code_line(file_content, e->hint_loc.line);
            print_err_msg(true, e->hint_loc, e->hint);
            printf("\n");
        }
    }
    return;
}

int main(int argc, char** argv)
{
    init_console();
    
    if (argc < 3) { 
        print_usage();
        exit(-1);
    }

    LARGE_INTEGER start_time;
    QueryPerformanceFrequency(&start_time);
    double freq = start_time.QuadPart;
    QueryPerformanceCounter(&start_time);

    arena = make_arena();
    char* command = argv[1];
    if (strcmp(command, "build") != 0) {
        log_fatal("Unrecognized command \"%s\"", command);
        print_usage(); exit(-1);
    }

    bool print_tokens = false;
    bool print_ast = false;
    int i;
    for (i = 3; i <= argc; i++) {
        char* opt = argv[i-1]; 
        if (strcmp("--print-tokens", opt) == 0) print_tokens = true;
        else if (strcmp("--print-ast", opt) == 0) print_ast = true;
        else break;
    }
    
    char* file_name = argv[i-1];
    char* file_content;
    int file_size = open_file(file_name, &file_content);

    errors = array_init(sizeof(error_t));
    file_names = array_init(sizeof(char*));
    char** slot = array_append(&file_names);
    *slot = file_name; 

    LARGE_INTEGER end_init;
    QueryPerformanceCounter(&end_init);
    
    // generate tokens
    token_t* start = arena.buckets[0]->cur;
    uint32_t token_count = lexer_tokenize(file_content, file_size, 0); // 0 => file_id
    if (print_tokens) lexer_debug(start, token_count);

    LARGE_INTEGER end_lex;
    QueryPerformanceCounter(&end_lex);

    // parse tokens
    ast_t* ast = parser_parse_tokens(start, token_count);
    LARGE_INTEGER end_parse;
    QueryPerformanceCounter(&end_parse);
    if (print_ast)    parser_debug(ast);
    if (array_len(&errors) > 0) {
        print_errors(file_content, file_size);
        goto compiler_end;
    }
    
    // resolve types and functions
    // check types
    // generate ir
    // optimize
    // register allocation && asm generation

compiler_end:
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    double all = (end.QuadPart - start_time.QuadPart) * 1000.f;
    all /= freq;
    double init = (end_init.QuadPart - start_time.QuadPart) * 1000.f;
    init /= freq;
    double lex = (end_lex.QuadPart - end_init.QuadPart) * 1000.f;
    lex /= freq;
    double parse = (end_parse.QuadPart - end_lex.QuadPart) * 1000.f;
    parse /= freq;
    log_info("Compilation took %3.3lfms (init: %3.3lfms, lex: %3.3lfms, parse: %3.3lfms)", all, init, lex, parse);
}