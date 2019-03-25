/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:25 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_TOOLS_H
# define MALLOC_TOOLS_H

# include "arenap.h"

void	*ft_memcpy(void *dst, void *src, size_t n);
void	update_max_chunk(t_bin *bin, t_chunk *next_chunk, size_t old_size);
t_arena	new_arena_default(size_t chunk_type, t_bin *bin);

void	*find_chunk(t_arena *arena, size_t size,
	size_t chunk_type, int zero_set);
t_bin	*create_new_bin(t_arena *arena, size_t size,
	size_t chunk_type, pthread_mutex_t *mutex);
void	*create_user_area (t_bin *bin, t_chunk *chunk,
	size_t size, int zero_set);
void	*first_cut(size_t size, size_t chunk_type,
	pthread_mutex_t *mtx, int zero_set);
void	*malloc_phase_2 (pthread_mutex_t *nmtx, size_t size,
	size_t chunk_type, int zero_set);

#endif
