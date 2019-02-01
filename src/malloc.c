#include "mallocp.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <ompt.h>


static t_arena	*g_main_arena = NULL;


static void
terminate (void) {
	munmap(g_main_arena, g_main_arena->size);
}

static t_arena *
create_new_arena (size_t size, long pagesize, pthread_mutex_t *mutex) {

	/* Our arena needs to contain the arena header and one header per chunk. Add these to the requested size. */
	static size_t headers_size = sizeof(t_arena) + sizeof(t_chunk) * CHUNKS_PER_ARENA;

	/*
	   Allocate memory using mmap. If the requested size isn't that big, we allocate enough memory to hold
	   CHUNKS_PER_ARENA times this size to prepare for future malloc. If the requested size exceeds that value, nearest
	   page size will do.
	*/
	if (size <= SIZE_SMALL) {
		size = size * 100 + headers_size;
	} else {
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

	printf("Thread %p is creating a new arena at address %p\n", (void *)pthread_self(), new_arena);

	pthread_mutex_lock(&new_arena->mutex);
	new_arena->size = rounded_size;
	return new_arena;
}

void *
__malloc (size_t size)
{
	static int				flags = 0, arena_count = 1;
	static long				pagesize = 0;
	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER, new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	t_arena					*current_arena = NULL;

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
		g_main_arena = create_new_arena(size, pagesize, &main_arena_mutex);
		if (g_main_arena == MAP_FAILED) return NULL;
		current_arena = g_main_arena;

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
		   If the next current_arena is NULL, all arenas are locked by other threads but there is still space to create
		   more. Let's just do that.
		*/

		if (creator == true) {

			/* Create a new arena and set it as the current arena. */
			current_arena->next = create_new_arena(size, pagesize, &new_arena_mutex);
			if (current_arena == MAP_FAILED) return NULL;
			current_arena = current_arena->next;

			/* If MAX_ARENA_COUNT is reached, point main_arena as the next entry to make the list circular. */
			if (++arena_count == MAX_ARENA_COUNT) current_arena->next = g_main_arena;

			pthread_mutex_unlock(&new_arena_mutex);

		}
	}

	pthread_mutex_unlock(&current_arena->mutex);

	return current_arena;
}
