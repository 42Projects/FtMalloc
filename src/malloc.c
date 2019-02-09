#include "mallocp.h"
#include <stdio.h>


t_arena_data		*g_arena_data = NULL;

static inline void *
user_area (t_bin *bin, t_chunk *chunk, size_t size, pthread_mutex_t *mutex) {

	/*
	   Populate chunk headers.
	   A chunk header precedes the user area with the size of the user area (to allow us to navigate between allocated
	   chunks which are not part of any linked list, and defragment the memory in free and realloc calls) and the pool
	   header address. We also use the size to store CHUNK_USED to know if the chunk is used or not.
	*/

	size += sizeof(t_chunk);
	bin->free_size -= size;

	unsigned long old_size = __mchunk_size(chunk);
	chunk->size = size | (1UL << CHUNK_USED);
	chunk->bin = bin;
	t_chunk *next_chunk = __mchunk_next(chunk);

	if (next_chunk != __mbin_end(bin) && next_chunk->size == 0) next_chunk->size = old_size - size;

	if (old_size == bin->max_chunk_size) {

		unsigned long req_size = __mchunk_type_match(bin, CHUNK_TINY) ? SIZE_TINY : SIZE_SMALL;

		if (next_chunk != __mbin_end(bin) && __mchunk_not_used(next_chunk) && __mchunk_size(next_chunk) >= req_size) {

			bin->max_chunk_size = __mchunk_size(next_chunk);
		} else {

			t_chunk *biggest_chunk = bin->chunk;
			unsigned long remaining = 0;
			while (biggest_chunk != __mbin_end(bin)) {

				if (__mchunk_not_used(biggest_chunk)) {

					if (__mchunk_size(biggest_chunk) > remaining) remaining = __mchunk_size(biggest_chunk);
					if (remaining >= req_size) break;
				}

				biggest_chunk = __mchunk_next(biggest_chunk);
			}

			bin->max_chunk_size = remaining;
		}

		__marena_update_max_chunks(bin);
	}

	pthread_mutex_unlock(mutex);
	return (void *)chunk->user_area;
}

static t_bin *
create_new_pool (t_arena *arena, int chunk_type, unsigned long size, long pagesize, pthread_mutex_t *mutex) {

	static unsigned long	headers_size = sizeof(t_bin) + sizeof(t_chunk);

	/*
	   Allocate memory using mmap. If the requested data isn't that big, we allocate enough memory to hold
	   CHUNKS_PER_POOL times this data to prepare for future malloc. If the requested data exceeds that value, nearest
	   page data will do.
	*/

	unsigned long mmap_size;

	if (chunk_type != CHUNK_LARGE) {
		mmap_size = sizeof(t_bin) + (size + sizeof(t_chunk)) * CHUNKS_PER_POOL;
	} else {
		mmap_size = size + headers_size;
	}

	mmap_size = mmap_size + (unsigned long)pagesize - (mmap_size % (unsigned long)pagesize);
	t_bin *pool = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	if (__builtin_expect(pool == MAP_FAILED, 0)) {
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return MAP_FAILED;
	}

	/* Keep track of the size and free size available. */
	pool->size = mmap_size | (1UL << chunk_type);
	pool->free_size = mmap_size - sizeof(t_bin);
	pool->arena = arena;
	pool->max_chunk_size = pool->free_size;
	pool->chunk->size = pool->free_size;
	__marena_update_max_chunks(pool);

	return pool;
}

void *
__malloc (size_t size) {

	static long 			pagesize = 0;
	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER,
							new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	static t_arena_data		arena_data = { .arena_count = 0 };


	size = (size + 0xfUL) & ~0xfUL;
	int chunk_type = 0;

	if (size >= (1UL << SIZE_THRESHOLD)) {
		return NULL;
	} else if (size > SIZE_SMALL) {
		chunk_type = CHUNK_LARGE;
	} else {
		chunk_type = (size <= SIZE_TINY) ? CHUNK_TINY : CHUNK_SMALL;
	}

	/*
	   First malloc may be called simultaneously by different threads. In order to prevent the main arena being created
	   more than once, we place a mutex before reading if main_arena already exists or not. If it doesn't, we release
	   the mutex after it's creation to enable other thread access. If it does, the mutex is released instantly.
	   GCC's builtin expect further improves performance as we know there will only be a single time in the program
	   execution where the comparison holds false.
	*/

	pthread_mutex_lock(&main_arena_mutex);

	if (__builtin_expect(g_arena_data == NULL, 0)) {
		pagesize = sysconf(_SC_PAGESIZE);

		t_bin *bin = create_new_pool(&arena_data.arenas[0], chunk_type, size, pagesize, &main_arena_mutex);
		if (bin == MAP_FAILED) return NULL;

		arena_data.arenas[0] = (t_arena){
			.small_bins = (chunk_type != CHUNK_LARGE) ? bin : NULL,
			.large_bins = (chunk_type == CHUNK_LARGE) ? bin : NULL,
			.max_chunk_small = 0,
			.max_chunk_tiny = 0
		};
		arena_data.arena_count = 1;
		pthread_mutex_init(&arena_data.arenas[0].mutex, NULL);
		pthread_mutex_lock(&arena_data.arenas[0].mutex);
		g_arena_data = &arena_data;

		pthread_mutex_unlock(&main_arena_mutex);
		return user_area(bin, bin->chunk, size, &arena_data.arenas[0].mutex);
	}

	/*
	   In order to prevent threads to compete for the same memory area, multiple arenas can be created to allow
	   for faster memory allocation in multi-threaded programs. Each arena has it's own mutex that will allow each
	   thread to operate independently. If M_ARENA_MAX is reached, threads will loop over all arenas until one
	   is available.
	*/

	pthread_mutex_unlock(&main_arena_mutex);

	/* Look for an open arena. */
	t_arena *arena = &arena_data.arenas[0];
	int arena_index = 0;
	while (pthread_mutex_trylock(&arena->mutex) != 0) {

		if (arena_index == arena_data.arena_count - 1) {

			if (pthread_mutex_trylock(&new_arena_mutex) == 0) {

				if (arena_data.arena_count < M_ARENA_MAX) {
					arena_index = arena_data.arena_count - 1;
					++arena_data.arena_count;
					arena = NULL;
					break;

				} else pthread_mutex_unlock(&new_arena_mutex);
			}

			arena = &arena_data.arenas[(arena_index = 0)];
			continue;
		}

		arena = &arena_data.arenas[arena_index++];
	}

	/* All arenas are occupied by other threads but M_ARENA_MAX isn't reached. Let's just create a new one. */
	if (arena == NULL) {

		arena = &arena_data.arenas[arena_index + 1];
		t_bin *bin = create_new_pool(arena, chunk_type, size, pagesize, &new_arena_mutex);
		if (bin == MAP_FAILED) return NULL;

		arena_data.arenas[arena_index + 1] = (t_arena){
				.small_bins = (chunk_type != CHUNK_LARGE) ? bin : NULL,
				.large_bins = (chunk_type == CHUNK_LARGE) ? bin : NULL,
				.max_chunk_small = 0,
				.max_chunk_tiny = 0
		};
		pthread_mutex_init(&arena->mutex, NULL);
		pthread_mutex_lock(&arena->mutex);

		pthread_mutex_unlock(&new_arena_mutex);
		return user_area(bin, bin->chunk, size, &arena->mutex);
	}

	/*
	   Otherwise, thread has accessed an arena and locked it. Now let's try to find a chunk of memory that is big
	   enough to accommodate the user-requested size.
	*/

	t_bin *bin = NULL;
	unsigned long required_size = size + sizeof(t_chunk);

	/* Look for a bin with a matching chunk type and enough space to accommodate user request. */
	if (chunk_type == CHUNK_LARGE || (chunk_type == CHUNK_TINY && arena->max_chunk_tiny >= required_size)
		|| (chunk_type == CHUNK_SMALL && arena->max_chunk_small >= required_size)) {

		bin = arena->small_bins;
		while (bin != NULL && bin->max_chunk_size < required_size) {
			bin = (chunk_type == CHUNK_TINY) ? bin->left : bin->right;
		}
	}

	if (bin == NULL) {

		/* A suitable bin could not be found, we need to create one. */
		bin = create_new_pool(arena, chunk_type, size, pagesize, &arena->mutex);
		if (bin == MAP_FAILED) return NULL;

		/*
		   As large bins generally don't need to be searched for empty space, they have their own linked list.
		   For tiny and small bins, we are using a non-circular double linked list anchored to the main bin.
		   Tiny chunks bins will be placed to the left of the main bin, and small chunks bins to it's right.
		*/

		t_bin *main_bin = (chunk_type == CHUNK_LARGE) ? arena->large_bins : arena->small_bins;

		if (main_bin != NULL) {
			t_bin *tmp = (chunk_type == CHUNK_TINY) ? main_bin->left : main_bin->right;

			/* Insert the new bin into the bin list. */
			if (chunk_type == CHUNK_TINY) {
				main_bin->left = bin;
				bin->right = main_bin;
				bin->left = tmp;

				if (tmp != NULL) tmp->right = bin;

			} else {
				main_bin->right = bin;
				bin->left = main_bin;
				bin->right = tmp;

				if (tmp != NULL) tmp->left = bin;
			}

		} else if (chunk_type == CHUNK_LARGE) {
			arena->large_bins = bin;
		} else {
			arena->small_bins = bin;
		}

		return user_area(bin, bin->chunk, size, &arena->mutex);
	}

	/*
	   Find the first free memory chunk and look for a chunk large enough to accommodate user request.
	   This loop is not protected, ie. if the chunk reaches the end of the bin it is undefined behavior.
	   However, a chunk should NEVER reach the end of the bin as that would mean that a chunk of suitable size could
	   not be found. If this happens, maths are wrong somewhere.
	*/

	t_chunk *chunk = bin->chunk;
	while (__mchunk_is_used(chunk) || __mchunk_size(chunk) < required_size) {
		chunk = __mchunk_next(chunk);
	}

	return user_area(bin, chunk, size, &arena->mutex);
}
