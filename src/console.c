#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include "include/console.h"

#define null NULL
bool use_color = false;

void init_console()
{   
    HANDLE hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hconsole == null) {
        return;
    }
    DWORD cur_mode;
    bool ok = GetConsoleMode(hconsole, &cur_mode);
    if (!ok) {
        printf("ERROR: failed to get console mode!\n");
        return;
    }

    ok = SetConsoleMode(hconsole, cur_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    if (!ok) {
        printf("ERROR: failed to set console mode!");
        return;
    }
    use_color = true;
}

const char* colors[] = {
    "\x1b[90m", "\x1b[32m", "\x1b[36m", "\x1b[33m", "\x1b[31m", "\x1b[91m"
};

const char* log_levels[] = {
    " DEBUG ", " INFO ", " WARN ", " ERROR ", " FATAL ",
};

void console_set_color(colors_e color)
{
    if (!use_color) return;
    printf("%s", colors[color]);
}

void console_set_bold()
{
    if (!use_color) return;
    printf("\x1b[1m");
}

void console_reset_bold()
{
    if (!use_color) return;
    printf("\x1b[22m");
}

void console_set_underline()
{
    if (!use_color) return;
    printf("\x1b[4m");
}

void console_reset_underline()
{
    if (!use_color) return;
    printf("\x1b[24m");
}

void console_reset(void)
{
    if (!use_color) return;
    printf("\x1b[0m");
}

void console_print_time()
{
    char buf[50];
    time_t raw_time;
    time(&raw_time);
    struct tm info;
    localtime_s(&info, &raw_time);
    strftime(buf, 49, "%X", &info);
    printf("%s", buf);
}

void _log_(char* file, int line, log_level_e level, char* fmt, ...)
{
    console_set_color(COLOR_GREY);
    console_print_time();

    console_set_color(level+1);
    console_set_bold();
    printf("%s", log_levels[level]);
    console_reset_bold();
    
    console_set_color(COLOR_GREY);
    printf("%s:%d ", file, line);
    console_reset();
    
    va_list args; va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    

    printf("\n");
}