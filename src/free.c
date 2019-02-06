#include "arenap.h"
#include "freep.h"
#include <stdio.h>
#include <string.h>


void
__free (void *ptr) {

	static unsigned long	headers_size = sizeof(t_pool) + sizeof(t_alloc_chunk) + sizeof(unsigned long);
	static pthread_mutex_t	arena_search_mutex = PTHREAD_MUTEX_INITIALIZER;


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
	pthread_mutex_lock(&arena_search_mutex);

	t_pool *main_pool = pool;
	while ((main_pool->size & (1UL << MAIN_POOL)) == 0) {
		main_pool = (pool_type_match(main_pool, CHUNK_TYPE_LARGE)) ? main_pool->right : main_pool->left;
	}

	t_arena *arena = &g_arena_data->arenas[0];
	int arena_index = 0;
	while (arena->main_pool != main_pool) {
		arena = &g_arena_data->arenas[++arena_index];
	}

	pthread_mutex_lock(&arena->mutex);

	if (pool_type_match(pool, CHUNK_TYPE_LARGE) == 0) {

	}

	/* If pool is empty, return memory to system. */
	if ((pool->free_size + chunk->size + headers_size) == (pool->size & SIZE_MASK)) {

		if (is_main_pool(pool)) {

			/* If the pool was the arena's only pool, we remove the arena altogether. */
			if (pool->left == NULL && pool->right == NULL) {

				while (arena_index < g_arena_data->arena_count - 1) {
					memmove(&g_arena_data->arenas[arena_index], &g_arena_data->arenas[arena_index + 1], sizeof(t_arena));
					++arena_index;
				}

				memset(&g_arena_data->arenas[arena_index], 0, sizeof(t_arena));

				if (--g_arena_data->arena_count == 0) g_arena_data = NULL;

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
	}

	pthread_mutex_unlock(&arena_search_mutex);
	pthread_mutex_unlock(&arena->mutex);
}
