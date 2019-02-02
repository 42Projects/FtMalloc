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


# define CHUNKS_PER_POOL 100
# define SIZE_TINY 256
# define SIZE_SMALL 4096
# define M_ARENA_MAX 16
# define SIZE_MASK ((1LL << FLAG_THRESHOLD) - 1)
# define FLAG_MASK (~SIZE_MASK)

enum					e_debug_flags {

	/* Will print general information. */
	DEBUG = 0,

	/* Will print even more information. */
	VERBOSE,
};

#endif /* __MALLOC_PRIVATE_H */
