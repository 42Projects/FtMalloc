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
        src/mallocp.h
        src/show.c
        src/showp.h
        src/arenap.h
        tests/test0.c
        tests/test1.c
        tests/multithreading.c)
