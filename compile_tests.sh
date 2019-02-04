gcc tests/multithreading.c -I include -l pthread build/malloc.o build/show.o -o multithreading_test
gcc tests/test0.c -o test0
gcc -L include tests/test1.c build/malloc.o -o test1
