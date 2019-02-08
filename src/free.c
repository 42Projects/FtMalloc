#include "arenap.h"
#include "freep.h"
#include <stdio.h>
#include <string.h>


static inline t_pool *
find_pool (t_chunk *chunk) {

	while ((chunk->prev_size & SIZE_MASK) != 0) {
		chunk = (t_chunk *)((unsigned long)chunk - (chunk->prev_size & SIZE_MASK));
	}

	t_pool *pool = (t_pool *)((unsigned long)chunk - sizeof(t_pool));
	pthread_mutex_lock(&pool->arena->mutex);

	return pool;
}

void
__free (void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *)((unsigned long)ptr - sizeof(t_chunk));

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if ((unsigned long)chunk % 16UL != 0 || chunk_is_allocated(chunk) == 0) {
		(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
		abort();
	}

	t_pool *pool = find_pool(chunk);
	t_arena *arena = pool->arena;

	/* We return the memory space to the pool free size. If the pool is empty, we unmap it. */
	pool->free_size += chunk->size;

	if ((pool->free_size + sizeof(t_pool)) == (pool->size & SIZE_MASK)) {

		if (is_main_pool(pool)) {

			if  (pool_type_match(pool, CHUNK_TYPE_LARGE)) {
				//TODO change
			}

			pool->chunk->size = pool->free_size;
			pool->chunk->prev_size = 0UL;

			memset(pool->chunk->user_area, 0, pool->free_size - sizeof(t_chunk));

		} else {

			if (pool->left != NULL) pool->left->right = pool->right;
			if (pool->right != NULL) pool->right->left = pool->left;

			munmap(pool, pool->size & SIZE_MASK);
		}

	} else {

		chunk->prev_size &= ~(1UL << USED_CHUNK);

		/* Defragment memory. */
		t_chunk *next_chunk = next_chunk(chunk);
//		if (chunk_is_allocated(next_chunk) == 0) {
//			chunk->size += next_chunk->size;
//		}




		memset(chunk->user_area, 0, chunk->size - sizeof(t_chunk));

		if (chunk->size > pool->biggest_chunk) pool->biggest_chunk = chunk->size;

		/* Update the chunk size in the next chunk header. Don't forget to keep the allocation bit. */
		if ((void *)next_chunk != pool_end(pool)) {
			next_chunk->prev_size &= (1UL << USED_CHUNK);
			next_chunk->prev_size |= chunk->size;
		}
	}

	pthread_mutex_unlock(&arena->mutex);
}
