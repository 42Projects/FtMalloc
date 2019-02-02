#include "mallocp.h"
#include <stdio.h>


t_arena	*g_main_arena = NULL;

static void
terminate (void) {
	t_arena *arena = g_main_arena, *next;

	do {
		next = arena->next;
		munmap(arena, ((t_pool *)(arena->pool))->size);
		arena = next;
	} while (next != NULL && next != g_main_arena);
}



static void *
create_new_pool (int type, int chunk_type, __uint64_t size, long pagesize, pthread_mutex_t *mutex) {

	/*
	   Our pool needs to contain the pool header and one header per chunk. We add 15 per chunk to that sum to align
	   the memory segment returned to the user on 16bytes boundaries. The first chunk will always be aligned regardless.
	*/

	static __uint64_t	arena_header = sizeof(t_arena),
						pool_header = sizeof(t_arena) + sizeof(t_pool),
						chunk_headers = (15 + sizeof(t_chunk)) * (CHUNKS_PER_POOL - 1) + sizeof(t_chunk);

	/*
	   Allocate memory using mmap. If the requested data isn't that big, we allocate enough memory to hold
	   CHUNKS_PER_POOL times this data to prepare for future malloc. If the requested data exceeds that value, nearest
	   page data will do.
	*/

	if (chunk_type == CHUNK_TYPE_LARGE) {
		size += pool_header + sizeof(t_chunk);
	} else {
		size = pool_header + size * 100 + chunk_headers;
	}

	/* We need to add the space for the arena header if we're creating an arena. */
	if (type == ARENA) size += arena_header;

	/* Round that size up to the nearest pagesize and call mmap. */
	__uint64_t rounded_size = size + pagesize - (size % pagesize);
	void *new_pool = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	/* Let's assume that usually the allocation will succeed. But in case it doesn't... */
	if (__builtin_expect(new_pool == MAP_FAILED, 0)) {
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return MAP_FAILED;
	}

	if (type == ARENA) pthread_mutex_lock(&((t_arena *)new_pool)->mutex);

	/* Keep track of the size and free size available. */
	t_pool *pool = (type == ARENA) ? ((t_arena *)new_pool)->pool : new_pool;
	pool->size = rounded_size | (1LL << chunk_type);

	/* Remaining size is irrelevant if the user requested a large chunk. */
	if (chunk_type != CHUNK_TYPE_LARGE) {
		pool->free_size = rounded_size - pool_header - sizeof(t_chunk) - size;

		if (type == ARENA) pool->free_size -= arena_header;
	}

	return new_pool;
}



void *
__malloc (size_t size)
{
	static int				flags = 0,
							arena_count = 1;
	static long 			pagesize = 0;
	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER,
							new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	int 					chunk_type = 0;
	bool					creator = false;
	t_arena					*current_arena = NULL;

	if (size >= (1LL << FLAG_THRESHOLD)) {
		return NULL;
	} else if (size <= SIZE_SMALL) {
		chunk_type = (size <= SIZE_TINY) ? CHUNK_TYPE_TINY : CHUNK_TYPE_SMALL;
	} else {
		chunk_type = CHUNK_TYPE_LARGE;
	}

	/*
	   First malloc may be called simultaneously by different threads. In order to prevent the main arena being created
	   more than once, we place a mutex before reading if main_arena already exists or not. If it doesn't, we release
	   the mutex after it's creation to enable other thread access. If it does, the mutex is released instantly.
	   GCC's builtin expect further improves performance as we know there will only be a single time in the program
	   execution where the comparison holds false.
	*/

	pthread_mutex_lock(&main_arena_mutex);

	if (__builtin_expect(g_main_arena == NULL, 0)) {

		/* Initialize non-runtime constant static variables. This will only be called once. */
		pagesize = sysconf(_SC_PAGESIZE);
		if (getenv("DEBUG") != NULL) flags |= 1 << DEBUG;
		if (getenv("VERBOSE") != NULL) flags |= 1 << VERBOSE;

		/* Initialize main arena and set it as the current arena. */
		g_main_arena = (t_arena *)create_new_pool(ARENA, chunk_type, size, pagesize, &main_arena_mutex);
		if (g_main_arena == MAP_FAILED) return NULL;
		current_arena = g_main_arena;

#if M_ARENA_MAX == 1
		g_main_arena->next_arena = g_main_arena;
#endif

		pthread_mutex_unlock(&main_arena_mutex);

		/* Register a call to munmap at program exit. */
		atexit(terminate);

		/* Return memory segment to the user. */
		t_pool *pool = (t_pool *)current_arena->pool;
		t_chunk *chunk = (t_chunk *)pool->chunk;
		chunk->size = (chunk_type == CHUNK_TYPE_LARGE) ? size : (size + 15) & ~0xf;
		pthread_mutex_unlock(&current_arena->mutex);

		return chunk->user_area;
	}

	/* Main arena exists, we can proceed. */

	pthread_mutex_unlock(&main_arena_mutex);

	/*
	   In order to prevent threads to compete for the same memory area, multiple arenas can be created to allow
	   for faster memory allocation in multi-threaded programs. Each arena has it's own mutex that will allow each
	   thread to operate independently. If M_ARENA_MAX is reached, threads will loop over all arenas until one
	   is available.
	*/

	/* Look for an open arena. */
	current_arena = g_main_arena;
	while (pthread_mutex_trylock(&current_arena->mutex) != 0) {

		if (current_arena->next == NULL) {

			/* We need to prevent several threads from creating new arenas simultaneously. */
			if (pthread_mutex_trylock(&new_arena_mutex) != 0) {
				current_arena = g_main_arena;
				continue;
			}

			creator = true;
			break;
		}

		current_arena = current_arena->next;
	}

	/*
	   If creator is true, all arenas are occupied by other threads but there is still space to create more.
	   Let's just do that.
	*/

	if (creator == true) {

		/* Create a new arena and set it as the current arena. */
		current_arena->next = (t_arena  *)create_new_pool(ARENA, chunk_type, size, pagesize, &new_arena_mutex);
		if (current_arena->next == MAP_FAILED) return NULL;
		current_arena = current_arena->next;

		/* If M_ARENA_MAX is reached, point main_arena as the next_arena entry to make the list circular. */
		if (++arena_count == M_ARENA_MAX) current_arena->next = g_main_arena;

		/* Return memory segment to the user. */
		t_pool *pool = (t_pool *)current_arena->pool;
		t_chunk *chunk = (t_chunk *)pool->chunk;
		chunk->size = (chunk_type == CHUNK_TYPE_LARGE) ? size : (size + 15) & ~0xf;
		pthread_mutex_unlock(&current_arena->mutex);

		return chunk->user_area;
	}

	/*
	   Thread has accessed an arena and locked it. Now let's try to find a chunk of memory that is big enough to
	   accommodate the user-requested size.
	*/

	t_pool *current_pool = (t_pool *)current_arena->pool;

	/*
	   Let's first handle the CHUNK_TYPE_LARGE case. As every chunk of this size has it's own dedicated pool, this case
	   merely requires us to create a new pool and send it's address back to the user.
	*/

	if (chunk_type == CHUNK_TYPE_LARGE) {

		/* Go to the end of the pool linked_list. */
		while (current_pool->next != NULL) {
			current_pool = current_pool->next;
		}

		/* Create a new pool and set it as the current pool. */
		current_pool->next = create_new_pool(POOL, CHUNK_TYPE_LARGE, size, pagesize, &current_arena->mutex);
		if (current_pool->next == MAP_FAILED) return NULL;
		current_pool->next->prev = current_pool;
		current_pool = current_pool->next;

		/* Return memory segment to the user. */
		t_chunk *chunk = (t_chunk *)current_pool->chunk;
		chunk->size = size;
		pthread_mutex_unlock(&current_arena->mutex);

		return chunk->user_area;
	}

	pthread_mutex_unlock(&current_arena->mutex);

	return current_arena;
}
