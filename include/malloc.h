#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

void	*calloc(size_t nmemb, size_t size);
void	free(void *ptr);
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);
void	show_alloc_mem(void);

#endif /* MALLOC_H */
