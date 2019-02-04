#ifndef __MALLOC_PRIVATE_H
# define __MALLOC_PRIVATE_H

# include "arenap.h"
# include <errno.h>
# include <sys/mman.h>

/* For getenv. */
# include <stdlib.h>

/* For sysconf and _SC_PAGESIZE. */
# include <unistd.h>

# define CHUNKS_PER_POOL 100
# define SIZE_TINY 256
# define SIZE_SMALL 4096
# define M_ARENA_MAX_DEFAULT 8
# define MEM_ALIGN 0xf

#endif /* __MALLOC_PRIVATE_H */
