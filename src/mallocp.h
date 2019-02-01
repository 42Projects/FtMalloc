#ifndef __MALLOC_PRIVATE_H
# define __MALLOC_PRIVATE_H

# include "arenap.h"

/* For errno */
# include <errno.h>

/* For bool. */
# include <stdbool.h>

/* For getenv and atexit. */
# include <stdlib.h>

/* For mmap and munmap. */
# include <sys/mman.h>

/* For sysconf and _SC_PAGESIZE. */
# include <unistd.h>


# define CHUNKS_PER_ARENA 100
# define SIZE_TINY 256
# define SIZE_SMALL 4096
# define MAX_ARENA_COUNT 16

enum					e_debug_flags {

	/* Will print general information. */
	DEBUG = 0,

	/* Will print even more information. */
	VERBOSE,
};

typedef __uint8_t 		user_ptr_t;

typedef struct			s_chunk {
	size_t 				size;
	user_ptr_t 			user_area[0];
}						t_chunk;

#endif /* __MALLOC_PRIVATE_H */
