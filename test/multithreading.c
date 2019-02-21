#include "malloc.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#define NUM_THREAD 4
#define FIRST_MALLOC_SIZE 40000
#define SECOND_MALLOC_SIZE 34
#define NUM(x) (x)
#define MALLOC(x) (malloc(x))
#define FREE(x) (free(x))
#define REALLOC(x, y) (realloc(x, y))
#define REALLOC_SIZE 5000


static void
*race_condition (void *info) {

	*(void **)info = MALLOC(NUM(FIRST_MALLOC_SIZE));

	pthread_exit(NULL);
}

static void
*second_call (void *info) {

	void *ret = MALLOC(NUM(SECOND_MALLOC_SIZE));
	memset(ret, 'a', SECOND_MALLOC_SIZE);
	FREE(ret);
	void *ptr1 = MALLOC(NUM(16));
	memset(ptr1, 'a', 16);
	void *ptr2 = MALLOC(NUM(16));
	memset(ptr2, 'a', 16);
	void *ptr3 = MALLOC(NUM(16));
	memset(ptr3, 'a', 16);
	FREE(ptr1);
	FREE(ptr2);
	void *ptr4 = MALLOC(NUM(16));
	memset(ptr4, 'a', 16);
	FREE(ptr4);
	FREE(ptr3);
	void *ptr5 = MALLOC(NUM(16));
	memset(ptr5, 'a', 16);
	void *ptr6 = MALLOC(NUM(16));
	memset(ptr6, 'a', 16);
	void *b = REALLOC(*(void **)info, NUM(REALLOC_SIZE));
	memset(b, 'a', REALLOC_SIZE);
	FREE(ptr6);
	FREE(ptr5);
	void *ptr7 = MALLOC(NUM(1096));
	memset(ptr7, 'a', 1096);
	FREE(ptr7);
	void *ptr8 = MALLOC(NUM(16));
	memset(ptr8, 'a', 16);
	FREE(ptr8);
	void *ptr9 = MALLOC(NUM(10056));
	memset(ptr9, 'a', 10056);
	FREE(ptr9);

	ptr1 = MALLOC(NUM(56));
	FREE(ptr1);
	ptr2 = MALLOC(NUM(96));
	FREE(ptr2);
	ptr3 = MALLOC(NUM(356));
	FREE(ptr3);

	ret = MALLOC(NUM(SECOND_MALLOC_SIZE));
	FREE(ret);
	ptr1 = MALLOC(NUM(56));
	ptr2 = MALLOC(NUM(96));
	ptr3 = MALLOC(NUM(356));
	FREE(ptr1);
	FREE(ptr2);
	ptr4 = MALLOC(NUM(2096));
	FREE(ptr3);
	ptr5 = MALLOC(NUM(256));
	ptr6 = MALLOC(NUM(196));

	b = REALLOC(b, NUM(REALLOC_SIZE));
	FREE(b);

	FREE(ptr6);
	FREE(ptr5);
	ptr7 = MALLOC(NUM(1096));
	FREE(ptr7);
	ptr8 = MALLOC(NUM(16));
	FREE(ptr8);
	ptr9 = MALLOC(NUM(10056));
	FREE(ptr9);

	ptr1 = MALLOC(NUM(56));
	FREE(ptr1);
	ptr2 = MALLOC(NUM(96));
	FREE(ptr2);
	ptr3 = MALLOC(NUM(356));
	FREE(ptr3);

	b = REALLOC(ptr4, NUM(3000));
	FREE(b);

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
	printf("Second batch of threads...\n");
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_create(th2 + k, NULL, second_call, info + k);
	}
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_join(th2[k], NULL);
	}

	show_alloc_mem();

	return 0;
}
