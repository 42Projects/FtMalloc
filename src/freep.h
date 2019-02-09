#ifndef __FREE_PRIVATE_H
# define __FREE_PRIVATE_H

# include "malloc.h"

/* For abort. */
# include <stdlib.h>

# define __mpool_main(bin) (__mchunk_type_match(bin, CHUNK_LARGE) ? bin == arena->large_bins : bin == arena->small_bins)

#endif /* __FREE_PRIVATE_H */
