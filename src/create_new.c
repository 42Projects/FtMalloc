/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:06 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc_tools.h"

t_arena	new_arena_default(size_t chunk_type, t_bin *bin)
{
	t_arena	arena;

	arena.tiny = (chunk_type == CHUNK_TINY) ? bin : NULL;
	arena.small = (chunk_type == CHUNK_SMALL) ? bin : NULL;
	arena.large = (chunk_type == CHUNK_LARGE) ? bin : NULL;
	arena.max_chunk_small = 0;
	arena.max_chunk_tiny = 0;
	return (arena);
}

t_bin	*create_new_bin(t_arena *arena, size_t size,
	size_t chunk_type, pthread_mutex_t *mutex)
{
	static size_t	headers_size = sizeof(t_bin) + sizeof(t_chunk);
	size_t			msz;
	t_bin			*bin;

	if (chunk_type != CHUNK_LARGE)
		msz = sizeof(t_bin) + (size + sizeof(t_chunk)) * CHUNKS_PER_POOL;
	else
		msz = size + headers_size;
	msz = msz + g_arena_data->pagesize - (msz % g_arena_data->pagesize);
	bin = mmap(NULL, msz, PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE, -1, 0);
	if (__builtin_expect(bin == MAP_FAILED, 0))
	{
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return (MAP_FAILED);
	}
	bin->size = msz;
	bin->type = chunk_type;
	bin->free_size = msz - sizeof(t_bin);
	bin->arena = arena;
	bin->max_chunk_size = bin->free_size;
	bin->chunk->size = bin->free_size;
	arena_update_max_chunks(bin, 0);
	return (bin);
}

void	*create_user_area(t_bin *bin, t_chunk *chunk,
	size_t size, int zero_set)
{
	size_t			old_size;
	t_chunk			*next_chunk;

	size += sizeof(t_chunk);
	bin->free_size -= size;
	old_size = chunk->size;
	chunk->size = size;
	chunk->used = 1;
	next_chunk = MCHUNK_NEXT(chunk);
	if (next_chunk != MBIN_END(bin) && old_size != size)
	{
		next_chunk->size = old_size - size;
		next_chunk->prev = chunk;
	}
	if (old_size == bin->max_chunk_size)
		update_max_chunk(bin, next_chunk, old_size);
	if (zero_set)
		ft_memset(chunk->user_area, 0, size - sizeof(t_chunk));
	else if (__builtin_expect(g_arena_data->env & M_SCRIBBLE, 0))
		ft_memset(chunk->user_area, 0xAA, chunk->size - sizeof(t_chunk));
	return (chunk->user_area);
}
