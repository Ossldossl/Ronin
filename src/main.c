#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>
#include <time.h>

#include "include/console.h"
#include "include/arena.h"
#include "include/array.h"
#undef HAS_ORIGINAL_ERRORS_ARRAY
#include "include/lexer.h"

#define null NULL

arena_t arena;
array_t errors;
array_t file_names;

#define HAS_ORIGINAL_ERRORS_ARRAY
#include "include/misc.h"

void print_usage(void)
{
    console_set_bold();
    printf("Ronin Compiler\n");
    console_reset_bold(); console_set_underline(); console_set_bold();
    printf("USAGE:\n");
    console_reset_bold(); console_reset_underline();
    printf("    ronin <command> [arguments]\n");
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
            char* line = malloc(len+1);
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

[[no_return]] void print_errors_and_exit(char* file_content, int file_size)
{
    for_array(&errors, error_t) 
        c_msg(e->error_loc, e->lvl, " %s", e->error);
        printf("    "); 
        char* error_line = get_line(file_content, e->error_loc.line);
        if (error_line == null) {
            log_error("Line is null!");
            continue;
        }
        printf("%s", error_line);
        printf("\n    "); 
        printf("\x1b[%dC", e->error_loc.col);
        console_set_bold(); console_set_color(COLOR_RED);
        for (int i = 0; i < e->error_loc.len; i++) {
            printf("^");
        }
        
        printf("-- ");
        printf("%s\n\n", e->error);
        console_reset();
    }
}

int main(int argc, char** argv)
{
    init_console();
    
    if (argc < 3) { 
        print_usage();
        exit(-1);
    }

    arena = arena_init();
    char* file_name = argv[2];
    char* file_content;
    int file_size = open_file(file_name, &file_content);

    errors = array_init(sizeof(error_t));
    file_names = array_init(sizeof(char*));
    char** slot = array_append(&file_names);
    *slot = file_name; 
    
    token_t* start = arena.first->cur;
    uint32_t token_count = lexer_tokenize(file_content, file_size, 0); // 0 => file_id
    lexer_debug(start, token_count);

    if (array_len(&errors) > 0) {
        print_errors_and_exit(file_content, file_size);

    }
    // parse tokens
    // type checking
    // ir generation
    // optimization
    // register allocation && asm generation
}