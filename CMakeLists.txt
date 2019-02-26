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
        src/show.c
        src/utils.c
        src/showp.h
        src/arenap.h
        test/test0.c
        test/test1.c
        test/test2.c
        test/test3.c
        test/test3_bis.c
        test/test4.c
        test/test5.c
        test/bonus_threading.c
        test/multithreading.c)
