#include "malloc.h"
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREAD 32000
#define FIRST_MALLOC_SIZE 42
#define SECOND_MALLOC_SIZE 4900
#define REALLOC_SIZE (FIRST_MALLOC_SIZE + SECOND_MALLOC_SIZE)

static void
*race_condition (void *info) {

//	*(void **)info = __malloc(FIRST_MALLOC_SIZE);

//	*(void **)info = __realloc(REALLOC_SIZE);

//	g_array[*(int *)info] = ret;

	pthread_exit(NULL);
}

static void
*second_call (void *info) {

//	void *ret = __malloc(SECOND_MALLOC_SIZE);

//	__free(ret);
//	__free(*(void **)info);

/*	void *ptr1 = __malloc(1230);
	void *ptr2 = __malloc(123);
	void *ptr3 = __malloc(130);
	__free(ptr1);
	__free(ptr2);
	void *ptr4 = __malloc(23);
	__free(ptr3);
	void *ptr5 = __malloc(12);
	void *ptr6 = __malloc(10);
	__free(ptr6);
	__free(ptr5);
	void *ptr7 = __malloc(120);
	__free(ptr7);
	void *ptr8 = __malloc(30);
	__free(ptr8);
	void *ptr9 = __malloc(230);
	__free(ptr9);
	__free(ptr4);*/

//	printf("ret = %p\n", ret);

	pthread_exit(NULL);
}

int
main (void) {

	pthread_t	th[NUM_THREAD];
	pthread_t	th2[NUM_THREAD];
	void		*info[NUM_THREAD];

	//setenv("DEBUG", "1", 1);

	/* Test for arena collision, true means there are race condition. */
	printf("First batch of threads is calling malloc of data %d...\n", FIRST_MALLOC_SIZE);
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_create(th + k, NULL, race_condition, info + k);
	}
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_join(th[k], NULL);
	}

	/* Test 2 */
	printf("Second batch of threads is calling malloc of data %d...\n", SECOND_MALLOC_SIZE);
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_create(th2 + k, NULL, second_call, info + k);
	}
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_join(th2[k], NULL);
	}

//	show_alloc_mem();

	return 0;
}