@echo off
cl test.c /fsanitize=address /Zi /Od /utf-8
@echo on
