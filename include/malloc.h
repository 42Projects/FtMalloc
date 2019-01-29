#ifndef MALLOC_H
# define MALLOC_H

# include "../libft/include/libft.h"
# include <errno.h>
# include <pthread.h>
# include <sys/mman.h>

# define TINY
# define SMALL
# define LARGE

# define TCACHE_FILL_COUNT 7

typedef struct		s_arena {
	pthread_mutex_t lock;
	__uint64_t		arenas;


	struct s_arena *next;
}					t_arena;

typedef struct		s_malloc {
	t_arena			*arena;
}					t_malloc;

void	free(void *ptr);
void	*__malloc(size_t size);
void	*realloc(void *ptr, size_t size);
void	show_alloc_mem(void);
void	show_alloc_mem_ex(void);

#endif
