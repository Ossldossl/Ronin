#pragma once
#include "console.h"
#include "misc.h"
#include "str.h"

size_t read_file(const char* file_name, char** file_content);
char is_dir(char* file_path);
bool file_exists(char* file_path);
char* get_cur_dir(void);
char* path_to_absolute(char* path, u32 len, u32* path_len);
str_t get_dir_name(char* file_path);;
str_t file_get_ident(char* path, u32 len);