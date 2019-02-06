#include "malloc.h"
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREAD 32
#define FIRST_MALLOC_SIZE 10
#define SECOND_MALLOC_SIZE 900

void	*g_array[NUM_THREAD];

static void
*race_condition (void *info) {
	void *ret = __malloc(FIRST_MALLOC_SIZE);

//	__free(ret);

	g_array[*(int *)info] = ret;

	pthread_exit(NULL);
}

static void
*second_call (void *info) {
	void *ret = __malloc(SECOND_MALLOC_SIZE);

//	printf("ret = %p\n", ret);

	pthread_exit(NULL);
}

int
main (void) {
	pthread_t	th[NUM_THREAD];
	pthread_t	th2[NUM_THREAD];
	int			info[NUM_THREAD];

	//setenv("DEBUG", "1", 1);

	/* Test for arena collision, true means there are race condition. */
	printf("First batch of threads is calling malloc of data %d...\n", FIRST_MALLOC_SIZE);
	for (int k = 0; k < NUM_THREAD; k++) {
		info[k] = k;
		pthread_create(th + k, NULL, race_condition, info + k);
	}
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_join(th[k], NULL);
	}

#if 0
	bool success = true;
	for (int k = 0; k < NUM_THREAD - 1; k++) {
		for (int p = k + 1; p < NUM_THREAD; p++) {
			if (g_array[p] == g_array[k]) {
				printf("Arena %p created by thread %p == arena %p created by thread %p\n", g_array[p], (void *)th[p], g_array[k], (void  *)th[k]);
				success = false;
				break;
			}
		}
	}
	dprintf(1, "Arena collision test: %s\x1b[0m\n", success == false ? "\x1b[1;34mFAIL" : "\x1b[32mPASS");
#endif
	/* Test 2 */
	printf("Second batch of threads is calling malloc of data %d...\n", SECOND_MALLOC_SIZE);
	for (int k = 0; k < NUM_THREAD; k++) {
		info[k] = k;
		pthread_create(th2 + k, NULL, second_call, info + k);
	}
	for (int k = 0; k < NUM_THREAD; k++) {
		pthread_join(th2[k], NULL);
	}

	show_alloc_mem();

	return 0;
}