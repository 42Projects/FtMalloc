#include "malloc.h"
#include "arenap.h"


t_arena_data	*g_arena_data = NULL;

static void
update_max_chunk (t_bin *bin, t_chunk *next_chunk, unsigned long old_size) {

	unsigned long req_size = __mbin_type_is(bin, CHUNK_TINY) ? SIZE_TINY : SIZE_SMALL;
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
	__marena_update_max_chunks(bin, old_size);
}

static t_bin *
create_new_bin (t_arena *arena, unsigned long size, int chunk_type, pthread_mutex_t *mutex) {

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

	mmap_size = mmap_size + g_arena_data->pagesize - (mmap_size % g_arena_data->pagesize);
	t_bin *bin = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	if (__builtin_expect(bin == MAP_FAILED, 0)) {
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return MAP_FAILED;
	}

	/* Keep track of the size and free size available. */
	bin->size = mmap_size | (1UL << chunk_type);
	bin->free_size = mmap_size - sizeof(t_bin);
	bin->arena = arena;
	bin->max_chunk_size = bin->free_size;
	bin->chunk->size = bin->free_size;
	__marena_update_max_chunks(bin, 0);

	return bin;
}

static void *
create_user_area (t_bin *bin, t_chunk *chunk, size_t size, int zero_set) {

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
	t_chunk *next_chunk = __mchunk_next(chunk);

	if (next_chunk != __mbin_end(bin) && old_size != size) {
		next_chunk->size = old_size - size;
		next_chunk->prev = chunk;
	}

	if (old_size == bin->max_chunk_size) update_max_chunk(bin, next_chunk, old_size);
	if (zero_set) memset(chunk->user_area, 0, size - sizeof(t_chunk));

	return chunk->user_area;
}

static void *
find_chunk (t_arena *arena, unsigned long size, int chunk_type, int zero_set) {

	t_bin *bin = (chunk_type == CHUNK_LARGE) ? arena->large : NULL;
	unsigned long required_size = size + sizeof(t_chunk);

	/* Look for a bin with a matching chunk type and enough space to accommodate user request. */
	if ((chunk_type == CHUNK_TINY && arena->max_chunk_tiny >= required_size)
		|| (chunk_type == CHUNK_SMALL && arena->max_chunk_small >= required_size)) {

		bin = __mbin_get_main(arena, chunk_type);
		while (bin != NULL && bin->max_chunk_size < required_size) {
			bin = bin->next;
		}
	}

	if (bin == NULL || (chunk_type == CHUNK_LARGE && bin->max_chunk_size < required_size )) {

		/* A suitable bin could not be found, we need to create one. */
		bin = create_new_bin(arena, size, chunk_type, &arena->mutex);
		if (bin == MAP_FAILED) return NULL;

		/*
		   As large bins generally don't need to be searched for empty space, they have their own linked list.
		   For tiny and small bins, we are using a non-circular double linked list anchored to the main bin.
		   Tiny chunks bins will be placed to the left of the main bin, and small chunks bins to it's right.
		*/

		t_bin *main_bin = __mbin_get_main(arena, chunk_type);
		if (main_bin != NULL) {
			t_bin *tmp = main_bin->next;

			/* Insert the new bin into the bin list. */
			main_bin->next = bin;
			bin->next = tmp;
			bin->prev = main_bin;

			if (tmp != NULL) tmp->prev = bin;

		} else if (chunk_type == CHUNK_TINY) {
			arena->tiny = bin;
		} else if (chunk_type == CHUNK_SMALL) {
			arena->small = bin;
		} else {
			arena->large = bin;
		}

		return create_user_area(bin, bin->chunk, size, zero_set);
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

	return create_user_area(bin, chunk, size, zero_set);
}

static void *
__malloc (size_t size, int zero_set) {

	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER,
							new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	static t_arena_data		arena_data = { .arena_count = 1, .env = 0 };


	/* Round the requested size to nearest 16 bytes. Allows us to always return aligned memory. */
	size = (size + 0xfUL) & ~0xfUL;

	int chunk_type;
	if (size >= (1UL << SIZE_THRESHOLD)) {
		return NULL;
	} else if (size > SIZE_SMALL) {
		chunk_type = CHUNK_LARGE;
	} else {
		chunk_type = (size <= SIZE_TINY) ? CHUNK_TINY : CHUNK_SMALL;
	}

	/* If first call to malloc, create our arena data struct. */
	pthread_mutex_lock(&main_arena_mutex);

	if (__builtin_expect(g_arena_data == NULL, 0)) {
		long pagesize = sysconf(_SC_PAGESIZE);
		arena_data.pagesize = (unsigned long)pagesize;
		g_arena_data = &arena_data;

		/* Get environment variables. */
		char *val = NULL;
		if ((val = getenv("M_ABORT_ON_ERROR")) != NULL && val[0] == '1') g_arena_data->env |= M_ABORT_ON_ERROR;
		if ((val = getenv("M_RELEASE_BIN")) != NULL && val[0] == '1') g_arena_data->env |= M_RELEASE_BIN;
		if ((val = getenv("M_SHOW_HEXDUMP")) != NULL && val[0] == '1') g_arena_data->env |= M_SHOW_HEXDUMP;
		if ((val = getenv("M_SHOW_UNALLOCATED")) != NULL && val[0] == '1') g_arena_data->env |= M_SHOW_UNALLOCATED;
		if ((val = getenv("M_SHOW_DEBUG")) != NULL && val[0] == '1') g_arena_data->env |= M_SHOW_DEBUG;

		t_bin *bin = create_new_bin(&arena_data.arenas[0], size, chunk_type, &main_arena_mutex);
		if (bin == MAP_FAILED) return NULL;

		arena_data.arenas[0] = (t_arena) {
			.tiny = (chunk_type == CHUNK_TINY) ? bin : NULL,
			.small = (chunk_type == CHUNK_SMALL) ? bin : NULL,
			.large = (chunk_type == CHUNK_LARGE) ? bin : NULL,
			.max_chunk_small = 0,
			.max_chunk_tiny = 0
		};
		pthread_mutex_init(&arena_data.arenas[0].mutex, NULL);
		pthread_mutex_lock(&arena_data.arenas[0].mutex);
		pthread_mutex_unlock(&main_arena_mutex);

		void *user_area = create_user_area(bin, bin->chunk, size, zero_set);
		pthread_mutex_unlock(&arena_data.arenas[0].mutex);
		return user_area;
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
				if (arena_index == arena_data.arena_count - 1 && arena_data.arena_count < M_ARENA_MAX) {
					arena = NULL;
					break;
				} else {
					pthread_mutex_unlock(&new_arena_mutex);
				}
			}
			arena_index = -1;
		}
		++arena_index;
		arena = &arena_data.arenas[arena_index];
	}

	/* All arenas are occupied by other threads but M_ARENA_MAX isn't reached. Let's just create a new one. */
	if (arena == NULL) {
		arena = &arena_data.arenas[arena_data.arena_count];
		t_bin *bin = create_new_bin(arena, size, chunk_type, &new_arena_mutex);
		if (bin == MAP_FAILED) return NULL;

		arena_data.arenas[arena_data.arena_count] = (t_arena) {
				.tiny = (chunk_type == CHUNK_TINY) ? bin : NULL,
				.small = (chunk_type == CHUNK_SMALL) ? bin : NULL,
				.large = (chunk_type == CHUNK_LARGE) ? bin : NULL,
				.max_chunk_small = 0,
				.max_chunk_tiny = 0
		};
		pthread_mutex_init(&arena->mutex, NULL);
		pthread_mutex_lock(&arena->mutex);
		++arena_data.arena_count;
		pthread_mutex_unlock(&new_arena_mutex);

		void *user_area = create_user_area(bin, bin->chunk, size, zero_set);
		pthread_mutex_unlock(&arena->mutex);
		return user_area;
	}

	/*
	   Otherwise, thread has accessed an arena and locked it. Now let's try to find a chunk of memory that is big
	   enough to accommodate the user-requested size.
	*/

	void *user_area = find_chunk(arena, size, chunk_type, zero_set);
	pthread_mutex_unlock(&arena->mutex);
	return user_area;
}

void *
realloc (void *ptr, size_t size) {

	if (ptr == NULL) {
		return malloc(size);
	} else {
		if (size == 0) free(ptr);
		if (size == 0 || size >= (1UL << SIZE_THRESHOLD)) return NULL;
	}

	t_chunk *chunk = (t_chunk *)ptr - 1;
	t_bin *bin = NULL;
	if (__builtin_expect(test_valid_chunk(chunk, &bin) != 0, 0)) {
		if (g_arena_data->env & M_ABORT_ON_ERROR) {
			(void)(write(STDERR_FILENO, "realloc(): invalid pointer\n", 27) << 1);
			abort();
		}

		return NULL;
	}

	if (__mchunk_size(chunk) - sizeof(t_chunk) >= size) return ptr;

	unsigned long req_size = ((size + 0xfUL) & ~0xfUL) + sizeof(t_chunk);
	t_arena *arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);
	t_chunk *next_chunk = __mchunk_next(chunk);

	/* If next chunk is available and the cumulative size is enough, extend the current chunk. */
	if (next_chunk != __mbin_end(bin) && __mchunk_not_used(next_chunk)
		&& __mchunk_size(chunk) + __mchunk_size(next_chunk) >= req_size) {
		unsigned long old_size = __mchunk_size(next_chunk);
		unsigned long realloc_size = __mchunk_size(chunk) + old_size;
		bin->free_size -= req_size - __mchunk_size(chunk);
		chunk->size = req_size | (1UL << CHUNK_USED);

		next_chunk = __mchunk_next(chunk);
		if (next_chunk != __mbin_end(bin) && req_size != realloc_size) {
			next_chunk->size = realloc_size - req_size;
			next_chunk->prev = chunk;
		}

		if (old_size == bin->max_chunk_size) update_max_chunk(bin, next_chunk, old_size);

		pthread_mutex_unlock(&arena->mutex);
		return chunk->user_area;
	}

	/* Otherwise, we have to find a new chunk, copy the content to it and free the current chunk. */
	size = req_size - sizeof(t_chunk);
	int chunk_type = (size > SIZE_SMALL) ? CHUNK_LARGE : (size <= SIZE_TINY) ? CHUNK_TINY : CHUNK_SMALL;
	void *user_area = find_chunk(arena, size, chunk_type, 0);
	memcpy(user_area, chunk->user_area, __mchunk_size(chunk) - sizeof(t_chunk));
	remove_chunk(bin, chunk);

	pthread_mutex_unlock(&arena->mutex);
	return user_area;
}

void *
malloc (size_t size) {

	return __malloc(size, 0);
}

void *
calloc (size_t nmemb, size_t size) {

	return __malloc(nmemb * size, 1);
}
