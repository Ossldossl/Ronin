@echo off
cl main.c lexer.c parser.c misc.c vector.c arena_alloc.c /fsanitize=address /Zi /Od /utf-8
del main.obj misc.obj
@echo on