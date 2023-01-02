@echo off
cl main.c misc.c vector.c lexer.c arena_alloc.c /fsanitize=address /Zi /Od /utf-8
del main.obj misc.obj
@echo on