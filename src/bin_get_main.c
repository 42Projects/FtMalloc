/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:28:49 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arenap.h"

t_bin	*bin_get_main(t_arena *arena, unsigned long chunk_type)
{
	if (chunk_type == CHUNK_TINY)
		return (arena->tiny);
	return ((chunk_type == CHUNK_SMALL) ? arena->small : arena->large);
}
