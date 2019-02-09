#include "arenap.h"
#include "freep.h"
#include <stdio.h>
#include <string.h>


void
__free (void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *)((unsigned long)ptr - sizeof(t_chunk));

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if ((unsigned long)chunk % 16UL != 0 || __mchunk_is_used(chunk) == 0) {
		(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
		abort();
	}

	t_pool *pool = chunk->pool;
	t_arena *arena = pool->arena;
	pthread_mutex_lock(&arena->mutex);

	/* We return the memory space to the pool free size. If the pool is empty, we unmap it. */
	pool->free_size += __mchunk_size(chunk);
	if (pool->free_size + sizeof(t_pool) == __mpool_size(pool)) {

		if (__mpool_main(pool)) {
			chunk = pool->chunk;
			chunk->size = pool->free_size;
			pool->max_chunk_size = chunk->size;

			if  (__mchunk_type_match(pool, CHUNK_LARGE)) {
				//TODO change

			} else __marena_update_max_chunks(pool);

			memset(chunk->user_area, 0, chunk->size - sizeof(t_chunk));

		} else {

			if (pool->left != NULL) pool->left->right = pool->right;
			if (pool->right != NULL) pool->right->left = pool->left;

			munmap(pool, pool->size & SIZE_MASK);
		}

	} else {
		chunk->size &= ~(1UL << CHUNK_USED);

		/* Defragment memory. */
		t_chunk *next_chunk = __mchunk_next(chunk);
		if (next_chunk != __mpool_end(pool) && __mchunk_not_used(next_chunk)) chunk->size += next_chunk->size;

		if (__mchunk_size(chunk) > pool->max_chunk_size) {
			pool->max_chunk_size = __mchunk_size(chunk);
			__marena_update_max_chunks(pool);
		}

		memset(chunk->user_area, 0, __mchunk_size(chunk) - sizeof(t_chunk));
	}

	pthread_mutex_unlock(&arena->mutex);
}
