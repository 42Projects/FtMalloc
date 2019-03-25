/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:34 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "showp.h"

static size_t	binslen(t_bin *bins[])
{
	size_t	ret;

	ret = 0;
	while (bins[ret])
		ret++;
	return (ret);
}

static size_t	second_cut(t_chunk *chunk, char *buffer, size_t *offset)
{
	size_t	chunk_size;

	chunk_size = chunk->size - sizeof(t_chunk);
	if ((chunk->used == 0 && g_arena_data->env & M_SHOW_UNALLOCATED)
		|| (g_arena_data->env & M_SHOW_HEXDUMP) == 0)
	{
		buff_number(18, (unsigned long)chunk->user_area, buffer, offset);
		buff_string(" - ", buffer, offset);
		buff_number(18, (unsigned long)MCHUNK_NEXT(chunk), buffer, offset);
		buff_string(" : ", buffer, offset);
		buff_number(10, chunk_size, buffer, offset);
		buff_string(" bytes", buffer, offset);
		if (chunk->used == 0)
			buff_string(" \x1b[92m(UNALLOCATED)\x1b[0m", buffer, offset);
		buffer[(*offset)++] = '\n';
	}
	if (chunk->used)
	{
		if (g_arena_data->env & M_SHOW_HEXDUMP)
			hexdump_chunk(chunk, buffer, offset);
		return (chunk_size);
	}
	return (0);
}

static void		first_cut(t_bin *bin, char *buffer, size_t *offset)
{
	if (bin->type == CHUNK_TINY)
		buff_string("\x1b[36mTINY :  ", buffer, offset);
	else if (bin->type == CHUNK_SMALL)
		buff_string("\x1b[36mSMALL : ", buffer, offset);
	else
		buff_string("\x1b[36mLARGE : ", buffer, offset);
	buff_number(18, (unsigned long)bin, buffer, offset);
	buff_string("\x1b[0m", buffer, offset);
	if (g_arena_data->env & M_SHOW_DEBUG)
	{
		buff_string(" [size = ", buffer, offset);
		buff_number(10, bin->size, buffer, offset);
		buff_string("], [free size = ", buffer, offset);
		buff_number(10, bin->free_size, buffer, offset);
		buff_string("], [max chunk size = ", buffer, offset);
		buff_number(10, bin->max_chunk_size, buffer, offset);
		buffer[(*offset)++] = ']';
	}
	buffer[(*offset)++] = '\n';
}

static size_t	explore_bins(t_bin *bins[], char *buffer, size_t *offset)
{
	size_t	arena_total;
	size_t	bin_num;
	size_t	k;
	t_bin	*bin;
	t_chunk	*chunk;

	bin_num = binslen(bins);
	arena_total = 0;
	k = 0;
	while (k < bin_num)
	{
		bin = bins[k];
		first_cut(bin, buffer, offset);
		chunk = bin->chunk;
		while (chunk != MBIN_END(bin))
		{
			if (chunk->used || g_arena_data->env & M_SHOW_UNALLOCATED)
				arena_total += second_cut(chunk, buffer, offset);
			chunk = MCHUNK_NEXT(chunk);
		}
		k++;
	}
	buff_string("\x1b[33mTotal (arena): \x1b[0m", buffer, offset);
	buff_number(10, arena_total, buffer, offset);
	return (arena_total);
}

void			show_alloc_mem(void)
{
	char	buffer[BUFF_SIZE];
	size_t	offset;
	size_t	gd_tot;
	int		k;
	t_arena	*arena;

	offset = 0;
	gd_tot = 0;
	k = -1;
	if (g_arena_data == NULL)
		return ;
	while (++k < g_arena_data->arena_count)
	{
		arena = &g_arena_data->arenas[k];
		pthread_mutex_lock(&arena->mutex);
		if (k != 0)
			buffer[offset++] = '\n';
		print_part1(arena, buffer, &offset, k);
		gd_tot += explore_bins(part2(arena, buffer, &offset), buffer, &offset);
		buff_string(" bytes\n", buffer, &offset);
		pthread_mutex_unlock(&arena->mutex);
	}
	last_block(gd_tot, buffer, &offset);
}
