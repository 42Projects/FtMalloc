#include "arenap.h"
#include "freep.h"
#include <stdio.h>
#include <string.h>


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
		chunk = (t_alloc_chunk *)((unsigned long)chunk - (previous_chunk_size(chunk) + sizeof(t_alloc_chunk)));
	}

	t_pool *pool = (previous_chunk_size(chunk) != 0) ?
			((t_free_chunk *)chunk)->head :
			(t_pool *)((unsigned long)chunk - sizeof(t_pool));

	/* Backtrack to the arena and lock it. */
	t_arena *arena = pool->arena;
	pthread_mutex_lock(&arena->mutex);

	/* We return the memory space to the pool free size. If the pool is empty, we unmap it. */
	pool->free_size += chunk->size + sizeof(t_alloc_chunk);

	if ((pool->free_size + headers_size) == (pool->size & SIZE_MASK)) {

		if (is_main_pool(pool)) {

			if (pool->left == NULL && pool->right == NULL) {
				arena->main_pool = NULL;

			} else {

				/* Assign the closest pool as the new main pool. */
				if (pool->right == NULL) arena->main_pool = pool->left;
				if (pool->left == NULL) arena->main_pool = pool->right;
				arena->main_pool->size |= (1UL << MAIN_POOL);
			}
		}

		if (pool->left != NULL) pool->left->right = pool->right;
		if (pool->right != NULL) pool->right->left = pool->left;

		munmap(pool, pool->size);
	} else if (pool_type_match(pool, CHUNK_TYPE_LARGE) == 0) {

		/* Otherwise, we attach the freed chunk to the pool free chunks list, and we populate t_free_chunk. */
		chunk->prev_size &= ~(1UL << USED_CHUNK);
		((t_free_chunk *)chunk)->head = pool;
//		((t_free_chunk *)chunk)->next = pool->
	}

	pthread_mutex_unlock(&arena->mutex);
}
