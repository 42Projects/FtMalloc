/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:09 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include "arenap.h"

static void	remove_chunk_cut2(t_bin *bin, t_chunk *chunk)
{
	t_chunk	*next_chunk;

	chunk->used = 0;
	next_chunk = MCHUNK_NEXT(chunk);
	if (next_chunk != MBIN_END(bin) && next_chunk->used == 0)
		chunk->size += next_chunk->size;
	next_chunk = MCHUNK_NEXT(chunk);
	if (next_chunk != MBIN_END(bin))
		next_chunk->prev = chunk;
	if (chunk->size > bin->max_chunk_size)
	{
		bin->max_chunk_size = chunk->size;
		arena_update_max_chunks(bin, 0);
	}
}

static int	remove_chunk_cut1(t_bin *bin, t_chunk **chunk)
{
	t_bin	*main_bin;

	main_bin = bin_get_main(bin->arena, bin->type);
	if ((__builtin_expect(g_arena_data->env & M_RELEASE_BIN, 0))
		&& main_bin != bin)
	{
		bin->prev->next = bin->next;
		if (bin->next != NULL)
			bin->next->prev = bin->prev;
		munmap(bin, bin->size);
		return (0);
	}
	else
	{
		*chunk = bin->chunk;
		(*chunk)->size = bin->free_size;
		(*chunk)->used = 0;
		bin->max_chunk_size = (*chunk)->size;
		if (bin->type != CHUNK_LARGE)
			arena_update_max_chunks(bin, 0);
	}
	return (1);
}

void		remove_chunk(t_bin *bin, t_chunk *chunk)
{
	bin->free_size += chunk->size;
	if (bin->free_size + sizeof(t_bin) == bin->size)
	{
		if (!remove_chunk_cut1(bin, &chunk))
			return ;
	}
	else
		remove_chunk_cut2(bin, chunk);
	if (__builtin_expect(g_arena_data->env & M_SCRIBBLE, 0))
	{
		ft_memset(chunk->user_area, 0x55, chunk->size - sizeof(t_chunk));
	}
}

void		free(void *ptr)
{
	t_chunk	*chunk;
	t_bin	*bin;
	t_arena	*arena;

	if (ptr == NULL)
		return ;
	chunk = (t_chunk *)ptr - 1;
	bin = NULL;
	if (__builtin_expect(test_valid_chunk(chunk, &bin) != 0, 0))
	{
		if (g_arena_data->env & M_ABORT_ON_ERROR)
		{
			(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) << 1);
			abort();
		}
		return ;
	}
	arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);
	remove_chunk(bin, chunk);
	pthread_mutex_unlock(&arena->mutex);
}
