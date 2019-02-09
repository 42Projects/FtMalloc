#include "malloc.h"

void *
calloc (size_t nmemb, size_t size) {

	return __malloc(nmemb * size);
}
