/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:27:52 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arenap.h"

void	arena_update_max_chunks(t_bin *bin, unsigned long old_size)
{
	if (bin->type == CHUNK_TINY
		&& (old_size == bin->arena->max_chunk_tiny
			|| bin->max_chunk_size > bin->arena->max_chunk_tiny))
		bin->arena->max_chunk_tiny = bin->max_chunk_size;
	else if (bin->type == CHUNK_SMALL
		&& (old_size == bin->arena->max_chunk_small
			|| bin->max_chunk_size > bin->arena->max_chunk_small))
		bin->arena->max_chunk_small = bin->max_chunk_size;
}
