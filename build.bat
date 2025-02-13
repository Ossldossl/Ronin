@echo off
set flags=-fsanitize=address -O0 -gfull -g3 -Wall -Wno-switch -Wno-microsoft-enum-forward-reference -Wno-unused-variable -Wno-unused-function 
set util_files=src/console.c src/arena.c src/array.c src/map.c src/str.c src/file.c
clang src/main.c src/lexer.c %util_files% -o out/main.exe %flags%
@echo on