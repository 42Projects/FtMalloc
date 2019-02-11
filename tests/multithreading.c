#include "malloc.h"
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREAD 32000
#define FIRST_MALLOC_SIZE 40
#define SECOND_MALLOC_SIZE 340
#define NUM(x) (x)
#define MALLOC(x) (__malloc(x))
#define FREE(x) (__free(x))
#define REALLOC_SIZE 5000

static void
*race_condition (void *info) {

	*(void **)info = MALLOC(NUM(FIRST_MALLOC_SIZE));

	pthread_exit(NULL);
}

static void
*second_call (void *info) {

	void *ret = MALLOC(NUM(SECOND_MALLOC_SIZE));
	FREE(ret);
	void *ptr1 = MALLOC(NUM(56));
	void *ptr2 = MALLOC(NUM(96));
	void *ptr3 = MALLOC(NUM(356));
	FREE(ptr1);
	FREE(ptr2);
	void *ptr4 = MALLOC(NUM(2096));
	FREE(ptr3);
	void *ptr5 = MALLOC(NUM(256));
	void *ptr6 = MALLOC(NUM(196));
//	void *b = __realloc(*(void **)info, NUM(REALLOC_SIZE));
	FREE(ptr6);
	FREE(ptr5);
	void *ptr7 = MALLOC(NUM(1096));
	FREE(ptr7);
	void *ptr8 = MALLOC(NUM(16));
	FREE(ptr8);
	void *ptr9 = MALLOC(NUM(10056));
	FREE(ptr9);
//	void *c = __realloc(ptr4, NUM(3000));

	pthread_exit(NULL);
}

int
main (void) {

	pthread_t	th[NUM_THREAD];
	pthread_t	th2[NUM_THREAD];
	void		*info[NUM_THREAD];

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
