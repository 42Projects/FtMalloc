/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:32:54 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include "malloc_tools.h"

static void	*last_option(size_t size, t_arena *arena,
	t_chunk *chunk[2], t_bin *bin)
{
	size_t	chunk_type;
	void	*user_area;

	if (size > SIZE_SMALL)
		chunk_type = CHUNK_LARGE;
	else
		chunk_type = (size <= SIZE_TINY) ? CHUNK_TINY : CHUNK_SMALL;
	user_area = find_chunk(arena, size, chunk_type, 0);
	ft_memcpy(user_area, chunk[0]->user_area, chunk[0]->size - sizeof(t_chunk));
	remove_chunk(bin, chunk[0]);
	pthread_mutex_unlock(&arena->mutex);
	return (user_area);
}

static void	*in_my_second_if(size_t req_size, t_arena *arena,
	t_chunk *chunk[2], t_bin *bin)
{
	size_t	old_size;
	size_t	realloc_size;

	old_size = chunk[1]->size;
	realloc_size = chunk[0]->size + old_size;
	bin->free_size -= req_size - chunk[0]->size;
	chunk[0]->size = req_size;
	chunk[0]->used = 1;
	chunk[1] = MCHUNK_NEXT(chunk[0]);
	if (chunk[1] != MBIN_END(bin) && req_size != realloc_size)
	{
		chunk[1]->size = realloc_size - req_size;
		chunk[1]->prev = chunk[0];
	}
	if (old_size == bin->max_chunk_size)
		update_max_chunk(bin, chunk[1], old_size);
	pthread_mutex_unlock(&arena->mutex);
	return (chunk[0]->user_area);
}

static void	*in_my_first_if(void)
{
	if (g_arena_data->env & M_ABORT_ON_ERROR)
	{
		(void)(write(STDERR_FILENO, "realloc(): invalid pointer\n", 27) << 1);
		abort();
	}
	return (NULL);
}

static void	*no_more_error(size_t size, t_chunk *chunk_src, t_bin *bin)
{
	size_t	req_size;
	t_arena	*arena;
	t_chunk	*chunk[2];

	req_size = ((size + 0xfUL) & ~0xfUL) + sizeof(t_chunk);
	arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);
	chunk[0] = chunk_src;
	chunk[1] = MCHUNK_NEXT(chunk[0]);
	if (chunk[1] != MBIN_END(bin) && chunk[1]->used == 0
		&& chunk[0]->size + chunk[1]->size >= req_size)
		return (in_my_second_if(req_size, arena, chunk, bin));
	size = req_size - sizeof(t_chunk);
	return (last_option(size, arena, chunk, bin));
}

void		*realloc(void *ptr, size_t size)
{
	t_chunk	*chunk;
	t_bin	*bin;

	if (ptr == NULL)
		return (malloc(size));
	else if (size > SIZE_THRESHOLD)
	{
		free(ptr);
		return (NULL);
	}
	else if (size == 0)
	{
		free(ptr);
		return (malloc(0));
	}
	chunk = (t_chunk *)ptr - 1;
	bin = NULL;
	if (__builtin_expect(test_valid_chunk(chunk, &bin) != 0, 0))
		return (in_my_first_if());
	if (chunk->size - sizeof(t_chunk) >= size)
		return (ptr);
	return (no_more_error(size, chunk, bin));
}
