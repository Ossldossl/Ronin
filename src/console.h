#pragma once

#include <stdbool.h>
#include "misc.h"

#define log_debug(...) _log_(__FILE__, __LINE__, LOG_DEBUG, __VA_ARGS__)
#define log_info(...) _log_(__FILE__, __LINE__, LOG_INFO, __VA_ARGS__)
#define log_warn(...) _log_(__FILE__, __LINE__, LOG_WARN, __VA_ARGS__)
#define log_error(...) _log_(__FILE__, __LINE__, LOG_ERROR, __VA_ARGS__)
#define log_fatal(...) _log_(__FILE__, __LINE__, LOG_FATAL, __VA_ARGS__)

#define c_info(loc, ...) c_print_error((loc), LOG_INFO, __VA_ARGS__)
#define c_warn(loc, ...) c_print_error((loc), LOG_WARN, __VA_ARGS__)
#define c_error(loc, ...) c_print_error((loc), LOG_ERROR, __VA_ARGS__)

#define c_msg(loc, level, ...) c_print_error((loc), (level), __VA_ARGS__)

typedef enum {
    COLOR_GREY = 0,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_YELLOW,
    COLOR_RED,
} colors_e;

void init_console();
void console_set_color(colors_e color);
void console_set_bold(void);
void console_reset_bold(void);
void console_set_underline(void);
void console_reset_underline(void);
void console_reset(void);
void console_print_time(void);
void _log_(char* file, int line, log_level_e level, char* fmt, ...);