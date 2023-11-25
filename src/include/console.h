#pragma once

#define log_debug(...) _log_(__FILE__, __LINE__, LOG_DEBUG, __VA_ARGS__)
#define log_info(...) _log_(__FILE__, __LINE__, LOG_INFO, __VA_ARGS__)
#define log_warn(...) _log_(__FILE__, __LINE__, LOG_WARN, __VA_ARGS__)
#define log_error(...) _log_(__FILE__, __LINE__, LOG_ERROR, __VA_ARGS__)
#define log_fatal(...) _log_(__FILE__, __LINE__, LOG_FATAL, __VA_ARGS__)

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
} log_level_e;

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
void _log_(char* file, int line, log_level_e level, char* fmt, ...);