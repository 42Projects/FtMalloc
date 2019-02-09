#ifndef __FREE_PRIVATE_H
# define __FREE_PRIVATE_H

# include "malloc.h"

/* For abort. */
# include <stdlib.h>

# define __mpool_main(pool) (pool->size & (1UL << MAIN_POOL))

#endif /* __FREE_PRIVATE_H */
