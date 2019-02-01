#include "mallocp.h"
#include <stdio.h>


t_arena
*g_main_arena = NULL;



static void
terminate (void) {
	t_arena *arena = g_main_arena, *next;

	do {
		next = arena->next;
		munmap(arena, arena->data & ((1LL << CHUNK_TYPE_TINY) - 1));
		arena = next;
	} while (next != NULL && next != g_main_arena);
}



static t_arena *
create_new_arena (size_t size, int type, long pagesize, pthread_mutex_t *mutex) {

	/* Our arena needs to contain the arena header and one header per chunk. Add these to the requested data. */
	static size_t headers_size = sizeof(t_arena) + sizeof(t_chunk) * CHUNKS_PER_ARENA;

	/*
	   Allocate memory using mmap. If the requested data isn't that big, we allocate enough memory to hold
	   CHUNKS_PER_ARENA times this data to prepare for future malloc. If the requested data exceeds that value, nearest
	   page data will do.
	*/
	int size_type;
	if (size <= SIZE_SMALL) {
		size_type = size <= SIZE_TINY ? CHUNK_TYPE_TINY : CHUNK_TYPE_SMALL;
		size = size * 100 + headers_size;
	} else {
		size_type = CHUNK_TYPE_LARGE;
		size += sizeof(t_arena) + sizeof(t_chunk);
	}
	size_t rounded_size = size + pagesize - (size % pagesize);
	t_arena *new_arena = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	/* Let's assume that usually the allocation will succeed. But in case it doesn't... */
	if (__builtin_expect(new_arena == MAP_FAILED, 0)) {
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return new_arena;
	}

	new_arena->data = rounded_size;
	new_arena->data |= 1LL << size_type;

	/* If arena is either main_arena or belong to it's linked list, toggle it's mutex and add a flag. */
	if (type == ARENA_TYPE_MAIN || type == ARENA_TYPE_LINKED) {
		pthread_mutex_lock(&new_arena->u_data.mutex);
		new_arena->data |= 1ULL << type;
	}

	return new_arena;
}



void *
__malloc (size_t size)
{
	static int				flags = 0, arena_count = 1;
	static long				pagesize = 0;
	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER, new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	t_arena					*current_arena = NULL;

	if (size >= 1LL << CHUNK_TYPE_TINY) return NULL;

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
		g_main_arena = create_new_arena(size, ARENA_TYPE_MAIN, pagesize, &main_arena_mutex);
		if (g_main_arena == MAP_FAILED) return NULL;
		current_arena = g_main_arena;

#if MAX_ARENA_COUNT == 1
		g_main_arena->next = g_main_arena;
#endif

		pthread_mutex_unlock(&main_arena_mutex);

		/* Register a call to munmap at program exit. */
		atexit(terminate);

	} else { /* Main arena exists, we can proceed. */

		pthread_mutex_unlock(&main_arena_mutex);

		/*
		   In order to prevent threads to compete for the same memory area, multiple arenas can be created to allow
		   for faster memory allocation in multi-threaded programs. Each arena has it's own mutex that will allow each
		   thread to operate independently. If MAX_ARENA_COUNT is reached, threads will loop over all arenas until one
		   is available.
		*/

		/* Look for an open arena. */
		bool creator = false;
		current_arena = g_main_arena;
		while (pthread_mutex_trylock(&current_arena->u_data.mutex) != 0) {
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
			current_arena->next = create_new_arena(size, ARENA_TYPE_LINKED, pagesize, &new_arena_mutex);
			if (current_arena == MAP_FAILED) return NULL;
			current_arena = current_arena->next;

			/* If MAX_ARENA_COUNT is reached, point main_arena as the next entry to make the list circular. */
			if (++arena_count == MAX_ARENA_COUNT) current_arena->next = g_main_arena;

			pthread_mutex_unlock(&new_arena_mutex);
		}
	}

	pthread_mutex_unlock(&current_arena->u_data.mutex);

	return current_arena;
}
