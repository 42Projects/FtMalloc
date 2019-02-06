#ifndef __MALLOC_PRIVATE_H
# define __MALLOC_PRIVATE_H

# include "malloc.h"
# include "arenap.h"
# include <errno.h>
# include <sys/mman.h>

# define CHUNKS_PER_POOL 100
# define SIZE_TINY 256
# define SIZE_SMALL 4096
# define MEM_ALIGN 0xfUL

#endif /* __MALLOC_PRIVATE_H */
