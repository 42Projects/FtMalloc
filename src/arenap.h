#ifndef __ARENA_PRIVATE_H
# define __ARENA_PRIVATE_H

# include <pthread.h>

# define is_main_arena(arena) (arena->data & (1LL << ARENA_TYPE_MAIN))
# define is_linked_arena(arena) (arena->data & (1LL << ARENA_TYPE_LINKED))
# define tiny_chunks_arena(arena) (arena->data & (1LL << CHUNK_TYPE_TINY))
# define small_chunks_arena(arena) (arena->data & (1LL << CHUNK_TYPE_SMALL))
# define large_chunks_arena(arena) (arena->data & (1LL << CHUNK_TYPE_LARGE))

enum					e_type {
	CHUNK_TYPE_TINY = 59,
	CHUNK_TYPE_SMALL,
	CHUNK_TYPE_LARGE,
	ARENA_TYPE_LINKED,
	ARENA_TYPE_MAIN
};

typedef __uint8_t		chunk_ptr_t;

typedef struct			s_arena {
	__uint64_t 			data;
	union {
		pthread_mutex_t	mutex;

	}					u_data;

	struct s_arena		*next;
	chunk_ptr_t 		chunk[0];
}						t_arena;

extern t_arena			*g_main_arena;

#endif /* __ARENA_PRIVATE_H */
