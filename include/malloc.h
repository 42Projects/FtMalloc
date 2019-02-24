#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

void	*__calloc(size_t nmemb, size_t size);
void	__free(void *ptr);
void	*__malloc(size_t size);
void	*__realloc(void *ptr, size_t size);
void	show_alloc_mem(void);

#endif /* MALLOC_H */
