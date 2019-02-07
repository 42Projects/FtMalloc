#include "malloc.h"
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREAD 3000
#define FIRST_MALLOC_SIZE 10000
#define SECOND_MALLOC_SIZE 1000

void	*g_array[NUM_THREAD];

static void
*race_condition (void *info) {
	*(void **)info = __malloc(FIRST_MALLOC_SIZE);

	__free(*(void **)info);

//	g_array[*(int *)info] = ret;

	pthread_exit(NULL);
}

static void
*second_call (void *info) {
//	void *ret = __malloc(SECOND_MALLOC_SIZE);

	__free(*(void **)info);

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