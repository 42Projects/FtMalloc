/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:18 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc_tools.h"

void		*ft_memcpy(void *dst, void *src, size_t n)
{
	size_t	k;

	k = 0;
	while (k < n)
	{
		((unsigned char *)dst)[k] = ((unsigned char *)src)[k];
		k++;
	}
	return (dst);
}

static void	*cut_find_chunk(t_bin **bin, t_arena *arena,
	size_t sz_cht[2], int zero_set)
{
	t_bin	*main_bin;
	t_bin	*tmp;

	*bin = create_new_bin(arena, sz_cht[0], sz_cht[1], &arena->mutex);
	if (*bin == MAP_FAILED)
		return (NULL);
	main_bin = bin_get_main(arena, sz_cht[1]);
	if (main_bin != NULL)
	{
		tmp = main_bin->next;
		main_bin->next = *bin;
		(*bin)->next = tmp;
		(*bin)->prev = main_bin;
		if (tmp != NULL)
			tmp->prev = *bin;
	}
	else if (sz_cht[1] == CHUNK_TINY)
		arena->tiny = *bin;
	else if (sz_cht[1] == CHUNK_SMALL)
		arena->small = *bin;
	else
		arena->large = *bin;
	return (create_user_area(*bin, (*bin)->chunk, sz_cht[0], zero_set));
}

void		update_max_chunk(t_bin *bin, t_chunk *nck, size_t old_size)
{
	size_t	req_size;
	size_t	remaining;
	t_chunk	*biggest_chunk;

	req_size = bin->type == CHUNK_TINY ? SIZE_TINY : SIZE_SMALL;
	if (nck != MBIN_END(bin) && nck->used == 0 && nck->size >= req_size)
		bin->max_chunk_size = nck->size;
	else
	{
		biggest_chunk = bin->chunk;
		remaining = 0;
		while (biggest_chunk != MBIN_END(bin))
		{
			if (biggest_chunk->used == 0)
			{
				if (biggest_chunk->size > remaining)
					remaining = biggest_chunk->size;
				if (remaining >= req_size)
					break ;
			}
			biggest_chunk = MCHUNK_NEXT(biggest_chunk);
		}
		bin->max_chunk_size = remaining;
	}
	arena_update_max_chunks(bin, old_size);
}

void		*find_chunk(t_arena *arena, size_t size,
	size_t chunk_type, int zero_set)
{
	t_bin	*bin;
	size_t	required_size;
	t_chunk	*chunk;
	size_t	sz_cht[2];

	bin = (chunk_type == CHUNK_LARGE) ? arena->large : NULL;
	required_size = size + sizeof(t_chunk);
	if ((chunk_type == CHUNK_TINY && arena->max_chunk_tiny >= required_size)
		|| (chunk_type == CHUNK_SMALL
			&& arena->max_chunk_small >= required_size))
	{
		bin = bin_get_main(arena, chunk_type);
		while (bin != NULL && bin->max_chunk_size < required_size)
			bin = bin->next;
	}
	sz_cht[0] = size;
	sz_cht[1] = chunk_type;
	if (bin == NULL
		|| (chunk_type == CHUNK_LARGE && bin->max_chunk_size < required_size))
		return (cut_find_chunk(&bin, arena, sz_cht, zero_set));
	chunk = bin->chunk;
	while (chunk->used || chunk->size < required_size)
		chunk = MCHUNK_NEXT(chunk);
	return (create_user_area(bin, chunk, size, zero_set));
}
