cmake_minimum_required(VERSION 3.13)
project(ft_malloc)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(include)
include_directories(src)
include_directories(tests)

add_executable(ft_malloc
        include/malloc.h
        src/free.c
        src/malloc.c
        src/malloc_private.h
        src/realloc.c
        tests/run.sh
        tests/test0.c
        main.c
        auteur
        ft_malloc.en.pdf
        Makefile
        README.md)
