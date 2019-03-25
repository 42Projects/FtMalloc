/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:41 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arenap.h"

static int	test_chunk(t_bin *bin, t_chunk *chunk)
{
	t_chunk *check;

	check = bin->chunk;
	while (check != MBIN_END(bin))
	{
		if (check == chunk)
			return (0);
		check = MCHUNK_NEXT(check);
	}
	return (2);
}

static int	test_bin(t_bin *bin, t_chunk *chunk, t_bin **lock_bin)
{
	while (bin != NULL)
	{
		if ((size_t)chunk > (size_t)bin
			&& (size_t)chunk < (size_t)MBIN_END(bin))
		{
			*lock_bin = bin;
			if (chunk->used == 0)
				return (2);
			return (test_chunk(bin, chunk));
		}
		bin = bin->next;
	}
	return (1);
}

static int	failed_test(int ret, pthread_mutex_t *mtx)
{
	pthread_mutex_unlock(mtx);
	return ((ret == 2) ? 1 : 0);
}

int			test_valid_chunk(t_chunk *chunk, t_bin **bin)
{
	int		k;
	int		ret;
	t_arena	*arena;

	if (g_arena_data == NULL || (unsigned long)chunk % 16UL != 0)
		return (1);
	k = -1;
	while (++k < g_arena_data->arena_count)
	{
		arena = &g_arena_data->arenas[k];
		pthread_mutex_lock(&arena->mutex);
		ret = test_bin(arena->tiny, chunk, bin);
		if (ret != 1)
			return (failed_test(ret, &arena->mutex));
		ret = test_bin(arena->small, chunk, bin);
		if (ret != 1)
			return (failed_test(ret, &arena->mutex));
		ret = test_bin(arena->large, chunk, bin);
		if (ret != 1)
			return (failed_test(ret, &arena->mutex));
		pthread_mutex_unlock(&arena->mutex);
	}
	return (1);
}
