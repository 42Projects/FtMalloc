#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <pthread.h>
# include <sys/mman.h>

# define M_ARENA_MAX 8
# define SIZE_THRESHOLD 58

enum					e_type {
	MAIN_POOL = SIZE_THRESHOLD + 1,
	CHUNK_USED,
	CHUNK_TINY,
	CHUNK_SMALL,
	CHUNK_LARGE
};

# define SIZE_MASK ((1UL << (SIZE_THRESHOLD + 1)) - 1)

# define __mchunk_is_used(chunk) (chunk->prev_size & (1UL << CHUNK_USED))
# define __mchunk_not_used(chunk) (__mchunk_is_used(chunk) == 0)
# define __mchunk_next(chunk) ((t_chunk *)((unsigned long)chunk + chunk->size))
# define __mchunk_type_match(pool, chunk_type) (pool->size & (1UL << chunk_type))
# define __mchunk_type_nomatch(pool, chunk_type) (__mchunk_type_match(pool, chunk_type) == 0)
# define __mpool_end(pool) ((void *)((unsigned long)pool + (pool->size & SIZE_MASK)))
# define __mpool_size(pool) (pool->size & SIZE_MASK)

# define __marena_update_max_chunks(pool)																		\
({ 																												\
	if (__mchunk_type_match(pool, CHUNK_TINY) && pool->max_chunk_size > pool->arena->max_chunk_tiny) { 			\
		pool->arena->max_chunk_tiny = pool->max_chunk_size; 													\
	} else if (__mchunk_type_match(pool, CHUNK_SMALL) && pool->max_chunk_size > pool->arena->max_chunk_small) { \
		pool->arena->max_chunk_small = pool->max_chunk_size; 													\
	} 																											\
})


typedef struct			s_chunk {
	unsigned long		prev_size;
	unsigned long		size;
	struct s_pool		*pool;
	__uint64_t 			__padding__;
	void				*user_area[0];
}						__attribute__((packed)) t_chunk;

typedef struct 			s_pool {
	unsigned long		free_size;
	unsigned long		size;
	unsigned long		max_chunk_size;
	struct s_arena		*arena;
	struct s_pool		*left;
	struct s_pool		*right;
	t_chunk				chunk[0];
}						__attribute__((packed)) t_pool;

typedef struct			s_arena {
	pthread_mutex_t		mutex;
	t_pool				*main_pool;
	unsigned long		max_chunk_tiny;
	unsigned long		max_chunk_small;
}						t_arena;

typedef struct 			s_arena_data {
	_Atomic int 		arena_count;
	t_arena				arenas[M_ARENA_MAX];
}						t_arena_data;

extern t_arena_data		*g_arena_data;

#endif /* __ARENA_PRIVATE_H */
