@echo off
clang main.c lexer.c parser.c misc.c vector.c arena_alloc.c -fsanitize=address -o ../out/main.exe -O0 -gfull -g3 -Wall -Wno-switch -Wno-microsoft-enum-forward-reference
@echo on