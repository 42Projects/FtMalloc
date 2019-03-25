/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:23 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc_tools.h"

static t_arena	*phase_2_cut(t_arena *arena, pthread_mutex_t *nmtx)
{
	int				arena_index;

	arena_index = 0;
	while (pthread_mutex_trylock(&arena->mutex) != 0)
	{
		if (arena_index == g_arena_data->arena_count - 1)
		{
			if (pthread_mutex_trylock(nmtx) == 0)
			{
				if (arena_index == g_arena_data->arena_count - 1
					&& g_arena_data->arena_count < M_ARENA_MAX)
				{
					arena = NULL;
					break ;
				}
				else
					pthread_mutex_unlock(nmtx);
			}
			arena_index = -1;
		}
		++arena_index;
		arena = &g_arena_data->arenas[arena_index];
	}
	return (arena);
}

void			*malloc_phase_2(pthread_mutex_t *nmtx, size_t size,
	size_t chunk_type, int zero_set)
{
	t_arena			*arena;
	t_arena_data	*adt;
	void			*user_area;
	t_bin			*bin;

	adt = g_arena_data;
	arena = &adt->arenas[0];
	arena = phase_2_cut(arena, nmtx);
	if (arena == NULL)
	{
		arena = &adt->arenas[adt->arena_count];
		bin = create_new_bin(arena, size, chunk_type, nmtx);
		if (bin == MAP_FAILED)
			return (NULL);
		adt->arenas[adt->arena_count++] = new_arena_default(chunk_type, bin);
		pthread_mutex_init(&arena->mutex, NULL);
		pthread_mutex_lock(&arena->mutex);
		pthread_mutex_unlock(nmtx);
		user_area = create_user_area(bin, bin->chunk, size, zero_set);
		pthread_mutex_unlock(&arena->mutex);
		return (user_area);
	}
	user_area = find_chunk(arena, size, chunk_type, zero_set);
	pthread_mutex_unlock(&arena->mutex);
	return (user_area);
}

void			*first_cut(size_t size, size_t chunk_type,
	pthread_mutex_t *mtx, int zero_set)
{
	t_arena_data	*arena_data;
	t_bin			*bin;
	void			*user_area;

	arena_data = g_arena_data;
	bin = create_new_bin(&(*arena_data).arenas[0], size, chunk_type, mtx);
	if (bin == MAP_FAILED)
		return (NULL);
	(*arena_data).arenas[0] = new_arena_default(chunk_type, bin);
	pthread_mutex_init(&(*arena_data).arenas[0].mutex, NULL);
	pthread_mutex_lock(&(*arena_data).arenas[0].mutex);
	pthread_mutex_unlock(mtx);
	user_area = create_user_area(bin, bin->chunk, size, zero_set);
	pthread_mutex_unlock(&(*arena_data).arenas[0].mutex);
	return (user_area);
}
