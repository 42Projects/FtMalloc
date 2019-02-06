#include "arenap.h"
#include "freep.h"
#include <stdio.h>


void
__free (void *ptr) {

	if (ptr == NULL)
		return;

	t_alloc_chunk *chunk = (t_alloc_chunk *)((unsigned long)ptr - sizeof(t_alloc_chunk));

	if (chunk_is_allocated(chunk) == 0 || (chunk->prev_size & CHECK_MASK) != 0 || ((unsigned long)ptr % 16) != 0) {
		(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
		abort();
	}

	/*
	   Backtrack to the pool header. We need to look for either an unallocated chunk which contains the address of the
	   pool header, or the first chunk of the pool which is defined by having a prev_size of 0.
	*/

	while (chunk_is_allocated(chunk) && previous_chunk_size(chunk) != 0) {
		chunk = (t_alloc_chunk *)((unsigned long)chunk - (previous_chunk_size(chunk) + sizeof(t_alloc_chunk)));
	}

	t_pool *pool = (previous_chunk_size(chunk) != 0) ?
			((t_free_chunk *)chunk)->head :
			(t_pool *)((unsigned long)chunk - sizeof(t_pool));

	if (pool_type_match(pool, CHUNK_TYPE_LARGE)) {

		if (pool->size & (1UL << MAIN_POOL)) printf("MAIN!\n");

		printf("LARGE!\n");
	}

}
