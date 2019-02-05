#ifndef __FREE_PRIVATE_H
# define __FREE_PRIVATE_H

# include "malloc.h"

/* For abort. */
# include <stdlib.h>

# define previous_chunk_size(chunk) (chunk->prev_size & ~(1UL << ALLOC_CHUNK))

# define CHECK_MASK (~((1UL << (FLAG_THRESHOLD + 1)) - 1) & ~(1UL << ALLOC_CHUNK))

#endif /* __FREE_PRIVATE_H */
