#ifndef MALLOC_PRIVATE_H
# define MALLOC_PRIVATE_H

# include <errno.h>
# include <pthread.h>
# include <stdlib.h>
# include <sys/mman.h>
# include <sys/resource.h>
# include <unistd.h>

# define TINY 256
# define SMALL 4096
# define THREAD_ID_OFFSET 0x83000

typedef struct		s_chunk {
	size_t 			size;
	void			*start;
}					t_chunk;

typedef struct		s_arena {

}					t_arena;

typedef struct		s_arena_map {
	pthread_t 		initial_thread;
	size_t 			size;
	t_arena			*map[];
}					t_arena_map;

#endif
