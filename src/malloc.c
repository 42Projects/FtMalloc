#include <malloc.h>
#include <stdio.h>


static t_arena	*g_malloc = NULL;

void	*__malloc(size_t size)
{
	static int		pagesize;
	static long		cpu_number;
	t_arena			current_arena;

	// Initialize main arena if undefined.
	if (g_malloc == NULL) {
		cpu_number = sysconf(_SC_NPROCESSORS_ONLN);
		pagesize = getpagesize();



		g_malloc = mmap(NULL, (size_t)pagesize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		if (g_malloc == MAP_FAILED) {
			errno = ENOMEM;
			return NULL;
		}

		current_arena = *g_malloc;
		current_arena.arenas = 1 | 1LL << 32; //TODO ?
		pthread_mutex_init(&current_arena.lock, NULL);
	}

	return NULL;
}
