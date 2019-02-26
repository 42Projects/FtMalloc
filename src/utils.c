#include "arenap.h"


void
ft_memset (void *b, int c, size_t len) {

	for (size_t k = 0; k < len; k++) {
		((unsigned char *)b)[k] = (unsigned char)c;
	}
}

size_t
malloc_good_size (size_t size) {

	return (size + 0xfULL) & ~0xfULL;
}

size_t
malloc_size (const void *ptr) {

	if (ptr == NULL) return 0;

	return (size_t)(((t_chunk *)ptr - 1)->size - sizeof(t_chunk));
}
