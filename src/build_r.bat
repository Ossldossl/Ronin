@echo off
clang main.c lexer.c file.c parser.c console.c allocators.c array.c map.c str.c -o ../out/main.exe -O3 -Wno-switch -Wno-microsoft-enum-forward-reference -Wno-unused-variable -Wno-unused-function
@echo on