gcc -g3 test/multithreading.c -I include -L. -pthread ./build/free.o ./build/malloc.o ./build/show.o -o multithreading_test
