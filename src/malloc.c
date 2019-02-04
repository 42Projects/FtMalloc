#include "mallocp.h"
#include <stdio.h>


t_arena	*g_main_arena = NULL;

static inline void *
user_area (t_pool *pool, t_alloc_chunk *chunk, size_t size, pthread_mutex_t *mutex) {

	/*
	   Populate chunk headers.
	   A chunk header precedes the user area with the size of the user area (to allow us to navigate between allocated
	   chunks which are not part of any linked list, and defragment the memory in free and realloc calls) and the size
	   of the previous chunk. We also use the previous size to store flags such as ALLOC_CHUNK to know if the chunk
	   is allocated or not.
	*/

	chunk->size = size;
	chunk->prev_size |= 1LL << ALLOC_CHUNK;

	/* If the end of the pool isn't reached, we also store the size of the current chunk into the next chunk header. */
	t_free_chunk *next_chunk = (t_free_chunk *)(chunk->user_area + size);
	if (pool_type_match(pool, CHUNK_TYPE_LARGE) == 0 && pool_end(pool) >= (__uint64_t)next_chunk + 8) {
		next_chunk->prev_size = size;
	}

	/* Unlock mutex and return memory segment to the user. */
	pthread_mutex_unlock(mutex);
	return (void *)chunk->user_area;
}

static void *
create_new_pool (int type, int chunk_type, __uint64_t size, long pagesize, pthread_mutex_t *mutex) {

	/*
	   Allocate memory using mmap. If the requested data isn't that big, we allocate enough memory to hold
	   CHUNKS_PER_POOL times this data to prepare for future malloc. If the requested data exceeds that value, nearest
	   page data will do.
	*/

	__uint64_t mmap_size = size;
	if (chunk_type == CHUNK_TYPE_LARGE) {
		mmap_size += sizeof(t_pool) + sizeof(t_alloc_chunk);
	} else {
		mmap_size = (chunk_type == CHUNK_TYPE_SMALL) ? SIZE_SMALL * 100 : SIZE_TINY * 100;
		mmap_size += sizeof(t_pool) + (MEM_ALIGN + sizeof(t_alloc_chunk)) * CHUNKS_PER_POOL;
	}

	/* We need to add the space for the arena header if we're creating an arena. */
	if (type == ARENA) mmap_size += sizeof(t_arena);

	/* Round that size up to the nearest pagesize and call mmap. */
	mmap_size = mmap_size + pagesize - (mmap_size % pagesize);
	void *new_pool = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	/* Let's assume that usually the allocation will succeed. But in case it doesn't... */
	if (__builtin_expect(new_pool == MAP_FAILED, 0)) {
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return MAP_FAILED;
	}

	/* Keep track of the size and free size available. */
	t_pool *pool = (type == ARENA) ? ((t_arena *)new_pool)->pool : new_pool;
	pool->end = (void *)(((__uint64_t)new_pool + mmap_size) | (1LL << chunk_type));

	/* Remaining size is irrelevant if the user requested a large chunk. */
	if (chunk_type != CHUNK_TYPE_LARGE) pool->free_size = mmap_size - (size + sizeof(t_alloc_chunk) + sizeof(t_pool));

	if (type == ARENA) {
		pool->end = (void *)((__uint64_t)pool->end | (1LL << MAIN_POOL));
		pool->free_size -= sizeof(t_arena);
		pthread_mutex_init(&((t_arena *)new_pool)->mutex, NULL);
		pthread_mutex_lock(&((t_arena *)new_pool)->mutex);
	}

	return new_pool;
}

void *
__malloc (size_t size) {

	static int				arena_count = 1;
	static long 			pagesize = 0;
	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER,
							new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	int 					chunk_type = 0;
	bool					creator = false;
	t_arena					*current_arena = NULL;
	t_pool					*current_pool = NULL;


	if (size > SIZE_SMALL) {
		chunk_type = CHUNK_TYPE_LARGE;
	} else {
		size = (size + MEM_ALIGN) & ~MEM_ALIGN;

		if (size >= (1LL << FLAG_THRESHOLD)) return NULL;

		chunk_type = (size <= SIZE_TINY) ? CHUNK_TYPE_TINY : CHUNK_TYPE_SMALL;
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

		/* Initialize main arena and set it as the current arena. */
		g_main_arena = (t_arena *)create_new_pool(ARENA, chunk_type, size, pagesize, &main_arena_mutex);
		if (g_main_arena == MAP_FAILED) return NULL;
		pthread_mutex_unlock(&main_arena_mutex);

#if M_ARENA_MAX == 1
		g_main_arena->next_arena = g_main_arena;
#endif

		current_pool = (t_pool *)g_main_arena->pool;

		return user_area(current_pool, (t_alloc_chunk *)current_pool->chunk, size, &g_main_arena->mutex);
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

		/* The list can only be non-circular if M_ARENA_MAX hasn't been reached yet. */
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

	/* All arenas are occupied by other threads but M_ARENA_MAX isn't reached. Let's just create a new one. */
	if (creator == true) {

		/* Create a new arena and set it as the current arena. */
		current_arena->next = (t_arena  *)create_new_pool(ARENA, chunk_type, size, pagesize, &new_arena_mutex);
		if (current_arena->next == MAP_FAILED) return NULL;
		current_arena = current_arena->next;

		/* If M_ARENA_MAX is reached, point the main arena as the next arena entry to make the list circular. */
		if (++arena_count == M_ARENA_MAX_DEFAULT) current_arena->next = g_main_arena;

		pthread_mutex_unlock(&new_arena_mutex);
		current_pool = (t_pool *)current_arena->pool;

		return user_area(current_pool, (t_alloc_chunk *)current_pool->chunk, size, &current_arena->mutex);
	}

	/*
	   Otherwise, thread has accessed an arena and locked it. Now let's try to find a chunk of memory that is big
	   enough to accommodate the user-requested size.
	*/

	current_pool = (t_pool *)current_arena->pool;
	__uint64_t required_size = size + sizeof(t_alloc_chunk);

	/*
	   Look for a pool with a matching chunk type and enough space to accommodate user request. This applies only if
	   the chunk type isn't large, as large chunks have their dedicated pool.
	*/

	if (chunk_type != CHUNK_TYPE_LARGE) {
		while (pool_type_match(current_pool, chunk_type) == 0 || current_pool->free_size < required_size) {

			if (current_pool->right == NULL) {
				creator = true;
				break;
			}

			current_pool = current_pool->right;
		}
	}

	/* A suitable pool could not be found, we need to create one. */
	if (chunk_type == CHUNK_TYPE_LARGE || creator == true) {

		/* Create a new pool and set it as the current pool. */
		current_pool = create_new_pool(POOL, chunk_type, size, pagesize, &current_arena->mutex);
		if (current_pool == MAP_FAILED) return NULL;

		/*
		   As large pools don't need to be searched for empty space, and because we want to place fresh tiny and small
		   pools the closest to the main pool to reduce access time, we are using a non-circular double linked list
		   anchored to the main pool. Large chunks pools will be placed to the left of the main pool, tiny and small
		   chunks to it's right.
		*/

		t_pool	*main_pool = (t_pool *)current_arena->pool,
				*tmp = (chunk_type == CHUNK_TYPE_LARGE) ? main_pool->left : main_pool->right ;

		/* Insert the new pool into the pool list. */
		if (chunk_type == CHUNK_TYPE_LARGE) {
			main_pool->left = current_pool;
			current_pool->right = main_pool;
			current_pool->left = tmp;

			if (tmp != NULL) tmp->right = current_pool;

		} else {
			main_pool->right = current_pool;
			current_pool->left = main_pool;
			current_pool->right = tmp;

			if (tmp != NULL) tmp->left = current_pool;
		}

		return user_area(current_pool, (t_alloc_chunk *)current_pool->chunk, size, &current_arena->mutex);
	}

	/* Find the first free memory chunk. */
	t_alloc_chunk *chunk = (t_alloc_chunk *)current_pool->chunk;
	while (chunk_is_allocated(chunk)) {
		chunk = (t_alloc_chunk *)(chunk->user_area + chunk->size);
	}

	/* Look for a chunk large enough to accommodate user request. */
	t_free_chunk *free_chunk = (t_free_chunk *)chunk, *anchor = NULL;
	while (free_chunk->next != NULL && free_chunk->size < required_size) {
		anchor = free_chunk;
		free_chunk = free_chunk->next;
	}

	/* Update pool remaining size and detach the chunk from the free chunks list. */
	current_pool->free_size -= required_size;
	if (anchor != NULL) {
		anchor->next = free_chunk->next;
		if (free_chunk->next) free_chunk->next->prev = anchor;
	}

	return user_area(current_pool, (t_alloc_chunk *)free_chunk, size, &current_arena->mutex);
}
