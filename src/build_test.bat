@echo off
clang test.c map.c -fsanitize=address -o ../out/test.exe -O0 -gfull -g3 -Wall -Wno-switch -Wno-microsoft-enum-forward-reference -Wno-unused-variable -Wno-unused-function
@echo on