@echo off
clang main.c arena.c -fsanitize=address -o ../out/main.exe -O0 -gfull -g3 -Wall -Wno-switch -Wno-microsoft-enum-forward-reference
@echo on