@echo off
clang main.c lexer.c parser.c console.c allocators.c array.c map.c str.c -fsanitize=address -o ../out/main.exe -O0 -gfull -g3 -Wall -Wno-switch -Wno-microsoft-enum-forward-reference -Wno-unused-variable -Wno-unused-function
@echo on