#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <errno.h>
# include <pthread.h>
# include <sys/mman.h>

/* For memset. */
# include <string.h>

/* For abort. */
# include <stdlib.h>


# define M_ARENA_MAX 8
# define CHUNKS_PER_POOL 100
# define SIZE_TINY 1024
# define SIZE_SMALL 4096
# define SIZE_THRESHOLD 59
# define SIZE_MASK ((1UL << (SIZE_THRESHOLD + 1)) - 1)

enum				e_type {
	CHUNK_USED = SIZE_THRESHOLD + 1,
	CHUNK_TINY,
	CHUNK_SMALL,
	CHUNK_LARGE
};

enum 				e_env {
	M_ABORT_ON_ERROR = 1,
	M_RELEASE_BIN = 2,
	M_SHOW_HEXDUMP = 4,
	M_SHOW_UNALLOCATED = 8
};

# define __marena_update_max_chunks(bin, old_size)													\
({ 																									\
	if (__mbin_type_is(bin, CHUNK_TINY) && (old_size == bin->arena->max_chunk_tiny					\
		|| bin->max_chunk_size > bin->arena->max_chunk_tiny)) {										\
		bin->arena->max_chunk_tiny = bin->max_chunk_size; 											\
	} else if (__mbin_type_is(bin, CHUNK_SMALL) && (old_size == bin->arena->max_chunk_small			\
		|| bin->max_chunk_size > bin->arena->max_chunk_small)) {									\
		bin->arena->max_chunk_small = bin->max_chunk_size; 											\
	}																								\
})
# define __mbin_end(bin) ((void *)((unsigned long)bin + __mbin_size(bin)))
# define __mbin_size(bin) (bin->size & SIZE_MASK)
# define __mchunk_is_used(chunk) (chunk->size & (1UL << CHUNK_USED))
# define __mchunk_next(chunk) ((t_chunk *)((unsigned long)chunk + __mchunk_size(chunk)))
# define __mchunk_not_used(chunk) (__mchunk_is_used(chunk) == 0)
# define __mchunk_size(chunk) (chunk->size & SIZE_MASK)
# define __mbin_type_is(bin, chunk_type) (bin->size & (1UL << chunk_type))
# define __mbin_type_not(bin, chunk_type) (__mbin_type_is(bin, chunk_type) == 0)

typedef struct		s_chunk {
	unsigned long	size;
	struct s_bin	*bin;
	void			*user_area[];
}					t_chunk;

typedef struct 		s_bin {
	unsigned long	free_size;
	unsigned long	size;
	unsigned long	max_chunk_size;
	struct s_arena	*arena;
	struct s_bin	*left;
	struct s_bin	*right;
	t_chunk			chunk[];
}					t_bin;

typedef struct		s_arena {
	pthread_mutex_t	mutex;
	t_bin			*small_bins;
	t_bin			*large_bins;
	unsigned long	max_chunk_tiny;
	unsigned long	max_chunk_small;
}					t_arena;

typedef struct 		s_arena_data {
	_Atomic int 	arena_count;
	t_arena			arenas[M_ARENA_MAX];
	int 			env;
	unsigned long 	pagesize;
}					t_arena_data;

extern t_arena_data	*g_arena_data;

void				remove_chunk(t_bin *bin, t_chunk *chunk, t_chunk *previous);
int 				test_valid_chunk(t_chunk *chunk, t_chunk **previous);

#endif /* __ARENA_PRIVATE_H */
