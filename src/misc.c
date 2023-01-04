#include "include/misc.h"
#include <string.h>
#include <io.h>
#include <fcntl.h>

// use arena for allocation
string_builder_t* stringb_new_warena(int length, rune* content, arena_allocator_t* arena) 
{
    string_builder_t* result = arena_alloc(arena);
    result->length = length;
    result->content = content;
    return result;
}

string_builder_t* stringb_new(int length, rune* content) 
{
    string_builder_t* result = malloc(sizeof(string_builder_t));
    result->length = length;
    result->content = content;
    return result;
}

// makes a copy of the string;
rune* stringb_to_cstring(string_builder_t* string_b) 
{
    rune* result = malloc((string_b->length + 1) * sizeof(rune));
    memcpy_s(result, (string_b->length + 1) * sizeof(rune), string_b->content, string_b->length * sizeof(rune));
    result[string_b->length] = '\0';
    return result;
}

void stringb_append(string_builder_t* stringb, rune* content_to_append, int length_of_appendix) 
{
    stringb->content = realloc(stringb->content, stringb->length + length_of_appendix + 1);
    memmove_s(&stringb->content[stringb->length + 1], length_of_appendix + 1, content_to_append, length_of_appendix);
    if (length_of_appendix > 0) {
        stringb->length += length_of_appendix;
    }
    stringb->content[stringb->length + 1] = '\0';
}

void stringb_free(string_builder_t* stringb) 
{
    free(stringb->content);
    free(stringb);
}

void print_message(int color, const char* const format, ...) {
    va_list argument_list;
    va_start(argument_list, format);

    wprintf(L"\x1B[%dm", color);
    vfprintf(stdout, format, argument_list);
    wprintf(L"\x1B[0m");

    va_end(argument_list);
}

static void print_error(char* file_path, error_t* error, vector_t* lines) 
{
    print_message(COLOR_RED, "%s\n", error->message);
    print_message(COLOR_BLUE, "-->\x20%s\n", file_path);
    
    if (error->line != 1) {
        print_message(COLOR_YELLOW, "%d| ", error->line - 1);
        rune* string_to_print1 = stringb_to_cstring(lines->data[error->line - 2]);
        print_message(COLOR_WHITE, "%ls\n", string_to_print1);
        free(string_to_print1);
    }

    print_message(COLOR_YELLOW, "%d| ", error->line);
    string_builder_t* line = lines->data[error->line - 1];

    string_builder_t part_before_error = {.content=line->content, .length=line->length - (line->length - error->col) };
    rune* string_to_print2 = stringb_to_cstring(&part_before_error);
    print_message(COLOR_WHITE, "%ls", string_to_print2);

    string_builder_t error_part = { &line->content[error->col], error->length };
    rune* string_to_print3 = stringb_to_cstring(&error_part);
    wprintf(L"\x1b[%dm", 1); // bold
    print_message(COLOR_RED, "%ls", string_to_print3);
    wprintf(L"\x1b[22m"); // removes bold

    string_builder_t part_after_error = { 
                                        &line->content[error->col + error->length], 
                                        line->length - (error->col - 1 + error->length) };
    rune* string_to_print4 = stringb_to_cstring(&part_after_error);
    print_message(COLOR_WHITE, "%ls\n", string_to_print4);

                      //digits of linenumber               //+1 => '|'; +1 => ' ';
    int num_of_digits = log10(error->line) + error->col + 3;
    // move cursor to the right by num_of_digits
    wprintf(L"\x1b[%dC", num_of_digits);
    
    // set textcolor to red (better than calling print_message over and over)
    wprintf(L"\x1b[%dm", COLOR_RED);
    for (int i = 0; i < error->length; i++) {
        printf("^");
    }
    printf(" %s\n", error->message);
    wprintf(L"\x1b[%dm", COLOR_WHITE);

    if (error->line != lines->used) {
        print_message(COLOR_YELLOW, "%d| ", error->line + 1);
        rune* string_to_print5 = stringb_to_cstring(lines->data[error->line]); // because error->line +1 -1
        print_message(COLOR_WHITE, "%ls\n", string_to_print5);
        free(string_to_print5);
    }

    free(string_to_print2);
    free(string_to_print3);
    free(string_to_print4);
}

static vector_t* parse_lines(rune* file_content, int file_size) 
{
    vector_t* result = vector_new();
    int   index       = 0;
    char  c           = -1;
    int   line_length = 0;
    rune* line_begin  = &file_content[0];
    while (true) 
    {
        c = file_content[index];
        if (c == '\0') {
            string_builder_t* string_b = stringb_new(line_length, line_begin);
            vector_push(result, string_b);
            break;
        }

        if (c == '\r') {
            index += 2;
            string_builder_t* string_b = stringb_new(line_length, line_begin);
            vector_push(result, string_b);
            line_begin = &file_content[index];
            line_length = 0;
            continue;
        } else if (c == '\n') {
            index++;
            string_builder_t* string_b = stringb_new(line_length, line_begin);
            vector_push(result, string_b);
            line_begin = &file_content[index];
            line_length = 0;
            continue;
        }
        line_length++;
        index++;
    }
    return result;
}

void print_errors(compiler_t* com) 
{
    vector_t* lines = parse_lines(arena_get(com->utf8_file_content, 0), len_arena(com->utf8_file_content) - 1);
    for (int i = 0; i < len(com->errors); i++) {
        print_error(com->file_path, com->errors->data[i], lines);
        printf("\n");
    }
}

__declspec(noreturn) void panic(const char* message) {
    print_message(COLOR_RED, message);
    exit(-1);
}

bool init_console(void) 
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) { print_message(COLOR_RED, "ERROR: Fehler beim abrufen von STD_OUTPUT: %lu", GetLastError()); }

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) { print_message(COLOR_RED, "ERROR: Fehler beim abrufen von GetConsoleMode: %lu", GetLastError()); }

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))  { print_message(COLOR_RED, "ERROR: Fehler beim abrufen von SetConsoleMode: %lu", GetLastError()); }

    // workaround für windows, da windows ein DOS 3.1 format benutzt ??
    // CP_ACP anstatt UTF_8 obwohl sie eigentlich das selbe sein sollten ??
	if (!SetConsoleOutputCP(CP_UTF7))
	{
        print_message(COLOR_YELLOW, "Warning: Failed to set Console to UTF-8 mode. Colors and Unicode characters might not be displayed.\n");
		return false;
	}
    system("chcp 65000"); // nur um sicher zu sein
    return true;
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
        print_message(COLOR_RED, "ERROR: Fehler beim %lcffnen der Datei: %s => %d", L'Ö', file_name, GetLastError());
        panic("\n");
    }

    return hFile;
}

int open_file(const char* file_name, char** file_content)
{
    HANDLE hFile = get_file_handle(file_name, false);
    LARGE_INTEGER file_size_li;
    if (GetFileSizeEx(hFile, &file_size_li) == 0) {
        print_message(COLOR_RED, "ERROR: Fehler beim Abfragen der Größe der Datei: %s", file_name);
        panic("\n");
    }
    int file_size = file_size_li.QuadPart;

    char* buf = malloc(file_size + 1);
    int read = 0;
    if (ReadFile(hFile, buf, file_size, &read, null) == 0 || read == 0) {
        print_message(COLOR_RED, "ERROR: Fehler beim Lesen der Datei: %s. Fehler: %lu", file_name, GetLastError());
        panic("\n");
    }
    buf[file_size] = '\0';
    CloseHandle(hFile);
    *file_content = buf;
    return file_size;
}

void write_file(const char* file_name, char* content, size_t size)
{
	HANDLE hFile = get_file_handle(file_name, true);
	bool result = WriteFile(hFile, content, size, null, null);
	if (!result)
	{
		print_message(COLOR_RED, "ERROR: Fehler beim Schreiben der Datei: %s. Fehler: %lu", file_name, GetLastError());
		panic("\n");
	}
}