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
# define MAX_ARENA_COUNT 16

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
	pthread_mutex_t	mutex;
	size_t 			size;

	struct s_arena	*next;
	char 			chunk[0];
}					t_arena;

#endif /* __MALLOC_PRIVATE_H */
