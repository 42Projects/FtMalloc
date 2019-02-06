#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <pthread.h>
# include <stdbool.h>
# include <sys/mman.h>

# define chunk_is_allocated(chunk) (chunk->prev_size & (1UL << USED_CHUNK))
# define pool_type_match(pool, chunk_type) (pool->size & (1UL << chunk_type))

# define M_ARENA_MAX 1
# define FLAG_THRESHOLD 58

enum					e_type {
	MAIN_POOL = FLAG_THRESHOLD + 1,
	USED_CHUNK,
	CHUNK_TYPE_TINY,
	CHUNK_TYPE_SMALL,
	CHUNK_TYPE_LARGE
};

typedef struct			s_free_chunk {
	unsigned long		prev_size;
	unsigned long		size;
	struct s_pool		*head;
	struct s_free_chunk	*next;
}						t_free_chunk;

typedef struct			s_alloc_chunk {
	unsigned long		prev_size;
	unsigned long		size;
	__uint8_t			user_area[0];
}						t_alloc_chunk;

typedef struct 			s_pool {
	unsigned long		free_size;
	unsigned long		size;
	struct s_pool		*left;
	struct s_pool		*right;
	__uint8_t			chunk[0];
}						t_pool;

typedef struct			s_arena {
	pthread_mutex_t		mutex;
	t_pool				*main_pool;
}						t_arena;

typedef struct 			s_arena_data {
	_Atomic int 		arena_count;
	t_arena				arenas[M_ARENA_MAX];
}						t_arena_data;

extern t_arena_data		*g_arena_data;
extern pthread_mutex_t	g_main_arena_mutex;

#endif /* __ARENA_PRIVATE_H */
