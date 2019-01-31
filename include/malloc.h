#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

void	__attribute__((visibility("hidden"))) debug(int code, int flags, void *a, void *b, void *c);
void	__free(void *ptr);
void	*__malloc(size_t size);
void	*__realloc(void *ptr, size_t size);
void	__attribute__((visibility("hidden"))) show_alloc_mem(void);
void	__attribute__((visibility("hidden"))) show_alloc_mem_ex(void);

#endif
