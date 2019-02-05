#include "arenap.h"
#include "freep.h"
#include <stdio.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void
__free (void *ptr) {

	if (ptr == NULL)
		return;

	t_alloc_chunk *chunk = (t_alloc_chunk *)((__uint64_t)ptr - sizeof(t_alloc_chunk));

	if (chunk_is_allocated(chunk) == 0 || (chunk->prev_size & CHECK_MASK) != 0 || ((__uint64_t)ptr % 16) != 0) {
		write(STDERR_FILENO, "free(): invalid pointer\n", 24);
		abort();
	}

	/*
	   Backtrack to the pool header. We need to look for either an unallocated chunk which contains the address of the
	   pool header, or the first chunk of the pool which is defined by having a prev_size of 0. Don't forget to toggle
	   off the allocated chunk flag in prev_size.
	*/

	while (chunk_is_allocated(chunk) && previous_chunk_size(chunk) != 0) {
		chunk = (t_alloc_chunk *)((__uint64_t)chunk - (previous_chunk_size(chunk) + sizeof(t_alloc_chunk)));
	}

	t_pool *pool = (previous_chunk_size(chunk) != 0) ? ((t_free_chunk *)chunk)->head : (t_pool *)((__uint64_t)chunk - sizeof(t_pool));

	if (pool_type_match(pool, CHUNK_TYPE_LARGE)) {
		printf("LARGE!\n");
	}

}
