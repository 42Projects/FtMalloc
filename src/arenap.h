#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <pthread.h>
# include <stdbool.h>

# define chunk_is_allocated(chunk) (chunk->prev_size & (1LL << ALLOC_CHUNK))
# define pool_type_match(pool, chunk_type) ((__uint64_t)pool->end & (1LL << chunk_type))
# define pool_end(pool) ((__uint64_t)pool->end & 0xffffffffffff)

# define FLAG_THRESHOLD 58

enum					e_type {
	ARENA,
	POOL,
	MAIN_POOL = FLAG_THRESHOLD + 1,
	ALLOC_CHUNK,
	CHUNK_TYPE_TINY,
	CHUNK_TYPE_SMALL,
	CHUNK_TYPE_LARGE
};

typedef struct			s_free_chunk {
	__uint64_t			prev_size;
	__uint64_t			size;
	struct s_free_chunk	*next;
	struct s_free_chunk	*prev;
}						t_free_chunk;

typedef struct			s_alloc_chunk {
	__uint64_t			prev_size;
	__uint64_t			size;
	__uint8_t			user_area[0];
}						t_alloc_chunk;

typedef struct 			s_pool {
	void				*end;
	__uint64_t 			free_size;
	struct s_pool		*left;
	struct s_pool		*right;
	__uint8_t			chunk[0];
}						t_pool;

typedef struct			s_arena {
	pthread_mutex_t		mutex;
	struct s_arena		*next;
	__uint8_t			pool[0];
}						t_arena;

extern t_arena			*g_main_arena;

#endif /* __ARENA_PRIVATE_H */
