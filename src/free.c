#include "arenap.h"
#include <stdio.h>

void	__free(void *ptr)
{
	if (ptr == NULL)
		return;

	t_alloc_chunk *chunk = (t_alloc_chunk *)((__uint64_t)ptr - sizeof(t_alloc_chunk));

	if (previous_chunk_size(chunk) == 0) printf("MAIN CHUNK\n");
}
