#include "arenap.h"
#include "freep.h"
#include <stdio.h>


void
__free (void *ptr) {

	static unsigned long	headers_size = sizeof(t_pool) + sizeof(unsigned long);


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
		chunk = (t_alloc_chunk *)((unsigned long)chunk - previous_chunk_size(chunk));
	}

	t_pool *pool = (previous_chunk_size(chunk) != 0) ?
			((t_free_chunk *)chunk)->head :
			(t_pool *)((unsigned long)chunk - sizeof(t_pool));

	/* Backtrack to the arena and lock it. */
	t_arena *arena = pool->arena;
	pthread_mutex_lock(&arena->mutex);

	/* We return the memory space to the pool free size. If the pool is empty, we unmap it. */
	pool->free_size += chunk->size;

	if ((pool->free_size + headers_size) == (pool->size & SIZE_MASK) && is_main_pool(pool) == 0) {

		if (pool->left != NULL) pool->left->right = pool->right;
		if (pool->right != NULL) pool->right->left = pool->left;

		munmap(pool, pool->size & SIZE_MASK);

	} else if (pool_type_match(pool, CHUNK_TYPE_LARGE) == 0) {

		/* Otherwise, we populate t_free_chunk. */
		chunk->prev_size &= ~(1UL << USED_CHUNK);
		((t_free_chunk *)chunk)->head = pool;
	}

	pthread_mutex_unlock(&arena->mutex);
}
