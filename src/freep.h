#ifndef __FREE_PRIVATE_H
# define __FREE_PRIVATE_H

# include "malloc.h"

/* For abort. */
# include <stdlib.h>

# define is_main_pool(pool) (pool->size & (1UL << MAIN_POOL))
# define previous_chunk_size(chunk) (chunk->prev_size & ~(1UL << USED_CHUNK))

# define SIZE_MASK ((1UL << (FLAG_THRESHOLD + 1)) - 1)
# define CHECK_MASK ((~SIZE_MASK) & ~(1UL << USED_CHUNK))

#endif /* __FREE_PRIVATE_H */
