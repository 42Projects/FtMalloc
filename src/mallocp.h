#ifndef __MALLOC_PRIVATE_H
# define __MALLOC_PRIVATE_H

# include "malloc.h"
# include <errno.h>
# include <fcntl.h>
# include <pthread.h>
# include <stdlib.h>
# include <sys/mman.h>
# include <unistd.h>

# define CHUNKS_PER_ARENA 100
# define SIZE_SMALL 4096
# define HASH_TABLE_SIZE 64

enum				e_debug_flags {

	/* Will print general information. */
	DEBUG = 0,

	/* Will print even more information. */
	VERBOSE,
};

enum				e_size {
	TINY = 0,
	SMALL,
	LARGE
};

typedef struct		s_chunk {
	size_t 			size;
	void			*start;
}					t_chunk;

typedef struct		s_arena {
	pthread_t 		id;

	struct s_arena	*next;
	char 			chunk[0];
}					t_arena;

typedef struct 		s_bucket {
	pthread_mutex_t	mutex;
	t_arena			*arena;
}					t_bucket;

typedef struct		s_arena_map {
	size_t 			size;
	t_bucket 		buckets[HASH_TABLE_SIZE];
}					t_arena_map;

#endif /* __MALLOC_PRIVATE_H */
