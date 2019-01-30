#include "malloc_private.h"
#include <stdio.h>
#include <string.h>


static t_arena_map		*g_arena_map = NULL;
static pthread_mutex_t	g_arena_mutex = PTHREAD_MUTEX_INITIALIZER;


void
terminate(void) {
	munmap(g_arena_map, g_arena_map->size);
}

t_arena *
create_new_arena(int pagesize, size_t size, pthread_t self) {
	static size_t headers_size = sizeof(t_arena) + sizeof(t_chunk) * 100;

	if (size <= SMALL) {
		size = size * 100 + headers_size;
	} else {
		size += sizeof(t_arena) + sizeof(t_chunk);
	}

	size_t rounded_size = size + pagesize - (size % pagesize);
	t_arena *new_arena = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

#if DEBUG == 0
	printf("Thread %p is creating a new arena\nArena start: %p\nSize: %lu\n", self, new_arena, rounded_size);
#endif

	return new_arena;
}

void *
__malloc(size_t size)
{
	pthread_t		self = pthread_self();
	static int		pagesize;
	t_arena			*current_arena = NULL;

	if (size <= 0) return NULL;

	// Lock arena mutex while thread find it's assigned arena
	pthread_mutex_lock(&g_arena_mutex);

	// First malloc call.
	if (__builtin_expect(g_arena_map == NULL, 0)) {
		pagesize = getpagesize();

		//Initialize arena map for faster arena access.
		struct rlimit resource;
		getrlimit(RLIMIT_NPROC, &resource);

		size_t map_size = resource.rlim_cur * sizeof(t_arena *) + sizeof(t_arena_map);
		size_t rounded_size = map_size + pagesize - (map_size % pagesize);

		g_arena_map = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		if (g_arena_map == MAP_FAILED) {
			errno = ENOMEM;
			return NULL;
		}

		memset(g_arena_map, 0, rounded_size);

		//Initialize main arena.
		current_arena = create_new_arena(pagesize, size, self);
		if (current_arena == MAP_FAILED) {
			errno = ENOMEM;
			return NULL;
		}

		g_arena_map->initial_thread = self;
		g_arena_map->size = rounded_size;
		g_arena_map->map[0] = current_arena;
		pthread_mutex_unlock(&g_arena_mutex);

		//Call munmap at program exit.
		atexit(terminate);
	} else {
		pthread_mutex_unlock(&g_arena_mutex);

		//Find the arena that matches local thread id.
		__darwin_ptrdiff_t target = ((unsigned int)self & 0xffffffff) - ((unsigned int)g_arena_map->initial_thread & 0xffffffff);
		target /= THREAD_ID_OFFSET;

		//Thread doesn't have an arena associated
		if (g_arena_map->map[target] == NULL) {
			current_arena = create_new_arena(pagesize, size, self);
			if (current_arena == MAP_FAILED) {
				errno = ENOMEM;
				return NULL;
			}

			g_arena_map->map[target] = current_arena;
		} else {
			current_arena = g_arena_map->map[target];
		}
	}

	return current_arena;
}
