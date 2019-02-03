#ifndef __MALLOC_PRIVATE_H
# define __MALLOC_PRIVATE_H

# include "arenap.h"
# include <sys/mman.h>
# include <errno.h>

/* For getenv and atexit. */
# include <stdlib.h>

/* For sysconf and _SC_PAGESIZE. */
# include <unistd.h>

# define CHUNKS_PER_POOL 100
# define SIZE_TINY 256
# define SIZE_SMALL 4096
# define M_ARENA_MAX 8
# define MEM_ALIGN 0xf

enum					e_debug_flags {

	/* Will print general information. */
	DEBUG = 0,

	/* Will print even more information. */
	VERBOSE,
};

#endif /* __MALLOC_PRIVATE_H */
