#pragma once
#include "console.h"
#include "misc.h"
#include "str.h"

bool set_current_directory(Str8 dir);
Str8 get_current_directory(void);
u64 read_file(const char* file_name, char** file_content);
char is_dir(char* file_path);
bool file_exists(char* file_path);
char* get_cur_dir(void);
char* path_to_absolute(char* path, u32 len, u32* path_len);
Str8 get_dir_name(char* file_path);
Str8 file_get_ident(char* path, u32 len);