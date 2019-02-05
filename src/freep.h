#ifndef __FREE_PRIVATE_H
# define __FREE_PRIVATE_H

# include "malloc.h"
# include <signal.h>

# define previous_chunk_size(chunk) (chunk->prev_size & ~(1ULL << ALLOC_CHUNK))

# define CHECK_MASK (~((1ULL << (FLAG_THRESHOLD + 1)) - 1) & ~(1ULL << ALLOC_CHUNK))

#endif /* __FREE_PRIVATE_H */
