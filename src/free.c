#include "arenap.h"
#include "freep.h"
#include <stdio.h>
#include <string.h>


t_pool *
check_valid_pointer (t_chunk *chunk) {

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if ((unsigned long)chunk % 16UL != 0) {
		(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
		abort();
	}

	/*
	   Go through arenas in order to find the chunk. The process is slow, but it will prevent us from reading memory
	   that doesn't belong to the program.
	*/

	t_pool *pool = NULL;
	t_chunk *check_chunk = NULL;
	for (int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];
		pthread_mutex_lock(&arena->mutex);

		/* We start by checking the pools of size tiny and small as there should be less of them on an average case. */
		t_pool *check_pool = arena->main_pool;
		while (check_pool != NULL) {

			if ((void *)chunk > (void *)check_pool && (void *)chunk < pool_end(check_pool)) {
				pool = check_pool;
				goto OUT;
			}

			check_pool = check_pool->right;
		}

		check_pool = arena->main_pool->left;
		while (check_pool != NULL) {

			if ((void *)chunk > (void *)check_pool && (void *)chunk < pool_end(check_pool)) {
				pool = check_pool;
				goto OUT;
			}

			check_pool = check_pool->left;
		}

		pthread_mutex_unlock(&arena->mutex);
	}

//	printf("TEST\n");

	OUT:
	if (pool != NULL) {

		/* Chunk is in a pool, but we still need to check that it actually is a valid chunk header. */
		check_chunk = pool->chunk;

//		printf("CHUNK = %p\n", chunk);

		while (check_chunk != pool_end(pool)) {

//			printf("CHECK_CHUNK = %p\n", check_chunk);

			if (chunk == check_chunk) {

				if (chunk_is_allocated(check_chunk)) return pool;

				(void)(write(STDERR_FILENO, "free(): double free or unallocated pointer\n", 43) + 1);
				abort();
			}

			check_chunk = next_chunk(check_chunk);
		}
	}

	(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
	abort();
}

void
__free (void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *) ((unsigned long)ptr - sizeof(t_chunk));
	t_pool *pool = check_valid_pointer(chunk);
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

		/* Try to defragment memory. */
//		t_chunk *next_chunk = (t_chunk *)((unsigned long)chunk + chunk->size);
//		if ()

		memset(chunk->user_area, 0, chunk->size - sizeof(t_chunk));
	}

	pthread_mutex_unlock(&arena->mutex);
}
