#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <pthread.h>
# include <sys/mman.h>

# define M_ARENA_MAX 1
# define FLAG_THRESHOLD 58

enum					e_type {
	MAIN_POOL = FLAG_THRESHOLD + 1,
	USED_CHUNK,
	CHUNK_TYPE_TINY,
	CHUNK_TYPE_SMALL,
	CHUNK_TYPE_LARGE
};

# define SIZE_MASK ((1UL << (FLAG_THRESHOLD + 1)) - 1)

# define chunk_is_allocated(chunk) (chunk->prev_size & (1UL << USED_CHUNK))
# define next_chunk(chunk) ((t_chunk *)((unsigned long)chunk + chunk->size))
# define pool_end(pool) ((void *)((unsigned long)pool + (pool->size & SIZE_MASK)))
# define pool_type_match(pool, chunk_type) (pool->size & (1UL << chunk_type))

typedef struct			s_chunk {
	unsigned long		prev_size;
	unsigned long		size;
	struct s_pool		*pool;
	__uint64_t 			__padding__;
	void				*user_area[0];
}						t_chunk;

typedef struct 			s_pool {
	unsigned long		free_size;
	unsigned long		size;
	unsigned long		biggest_chunk_size;
	struct s_arena		*arena;
	struct s_pool		*left;
	struct s_pool		*right;
	t_chunk				chunk[0];
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

#endif /* __ARENA_PRIVATE_H */
