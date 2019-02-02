#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <pthread.h>

# define pool_is_empty(pool) (pool->size & (1LL << EMPTY_POOL))
# define pool_type_match(pool, chunk_type) (pool->size & (1LL << chunk_type))

# define FLAG_THRESHOLD 60

enum				e_type {
	ARENA,
	POOL,
	CHUNK_TYPE_TINY = FLAG_THRESHOLD + 1,
	CHUNK_TYPE_SMALL,
	CHUNK_TYPE_LARGE
};

typedef struct		s_chunk {
	__uint64_t		size;
	struct s_chunk	*next;
	struct s_chunk	*prev;
	__uint64_t 		__padding;
	__uint8_t		user_area[0];
}					t_chunk;

typedef struct 		s_pool {
	__uint64_t 		size;
	__uint64_t 		free_size;
	struct s_pool	*next;
	struct s_pool	*prev;
	__uint8_t		chunk[0];
}					t_pool;

typedef struct		s_arena {
	pthread_mutex_t	mutex;
	struct s_arena	*next;
	__uint8_t		pool[0];
}					t_arena;

extern t_arena		*g_main_arena;

#endif /* __ARENA_PRIVATE_H */
