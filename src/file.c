#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj_core.h>
#include <stdlib.h>
#include "file.h"
#include "arena.h"
#include "str.h"

extern Arena arena;

bool set_current_directory(Str8 dir) {
    return SetCurrentDirectoryA(str_to_cstr(&dir));
}

Str8 get_current_directory(void) {
    char* buf = arena_alloc(&arena, 512);
    u32 size = GetCurrentDirectoryA(512, buf);
    arena_free_last(&arena);
    arena_alloc(&arena, size);
    return make_str(buf, size);
}

static HANDLE get_file_handle(const char* file_name, bool write) 
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

size_t read_file(const char* file_name, char** file_content)
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

// 0 => path does not exist; 1 => path points to a file; 2 => path points to a dir
char is_dir(char* file_path)
{
    DWORD attrs = GetFileAttributesA(file_path);
    if (attrs == 0) {
        log_fatal("Path does not exist!");
        exit(-2);
    }
    return attrs & FILE_ATTRIBUTE_DIRECTORY;
}

bool file_exists(char* file_path)
{
    DWORD attrs = GetFileAttributesA(file_path);
    if (attrs == 0 || attrs & FILE_ATTRIBUTE_DIRECTORY) return false;
    return true;
}

char* get_cur_dir(void)
{
    DWORD size = GetModuleFileName(0, null, 0);
    char* result = arena_alloc(&arena, size);
    GetModuleFileName(0, result, size);
    return result;
}

wchar_t* multi_to_wide_char(char* path, u32 len) {
    int size = MultiByteToWideChar(CP_UTF8, 0, path, len, null, 0);
    if (size == 0) return null;
    wchar_t* result = arena_alloc(&arena, size);
    MultiByteToWideChar(CP_UTF8, 0, path, len, result, size);
    return result;
}

char* path_to_absolute(char* path, u32 len, u32* path_len)
{
    char* result = arena_alloc(&arena, MAX_PATH+1);
    DWORD size = GetFullPathNameA(path, MAX_PATH, result, null);
    arena.buckets[arena.bucket_count-1]->cur -= (MAX_PATH) - size;
    *path_len = size;
    return result;
}

Str8 get_dir_name(char* file_path) 
{
    Str8 result;
    result.data = file_path;
    result.len = strlen(file_path);
    char* end = result.data + result.len;
    while (*end != '/' && *end != '\\') {
        end--; result.len--;
    }
    return result;
}

Str8 file_get_ident(char* path, u32 len)
{
    Str8 result; result.len = 0;
    char* c = path+len - 3;  // minus .rn
    while (true) {
        if (*c == '/' || *c == '\\') break; 
        result.len++;
        c--;
    }
    result.data = c+1;
    result.len -= 1;
    return result;
}