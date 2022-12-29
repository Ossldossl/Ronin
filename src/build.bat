@echo off
cl main.c misc.c vector.c lexer.c arena_alloc.c /fsanitize=address /Zi
del main.obj misc.obj
@echo on