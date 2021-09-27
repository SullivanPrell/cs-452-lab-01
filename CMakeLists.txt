cmake_minimum_required(VERSION 3.0)
project(cs_452_lab_01 C)

set(CMAKE_C_STANDARD 99)

add_executable(cs_452_lab_01 main.c myshell.c test.c lex.c)
