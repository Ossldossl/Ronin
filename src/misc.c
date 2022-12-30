#include "include/misc.h"

void print_message(int color, const char* const format, ...) {
    va_list argument_list;
    va_start(argument_list, format);

    wprintf(L"\x1B[%dm", color);
    vfprintf(stderr, format, argument_list);
    wprintf(L"\x1B[0m");

    va_end(argument_list);
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

	if (!SetConsoleOutputCP(CP_UTF8))
	{
        print_message(COLOR_YELLOW, "Warning: Failed to set Console to UTF-8 mode. Colors might not be displayed.\n");
		return false;
	}

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
        print_message(COLOR_RED, "ERROR: Fehler beim Öffnen der Datei: %s => %d", file_name, GetLastError());
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