/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:36 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "showp.h"

static void		step(t_bin *block, t_bin **bins, size_t *index)
{
	size_t	q;
	size_t	r;
	t_bin	*bin;

	bin = block;
	while (bin != NULL)
	{
		q = 0;
		while (q < *index)
		{
			if (bins[q] > bin)
			{
				r = *index + 1;
				while (--r > q)
					bins[r] = bins[r - 1];
				break ;
			}
			q += 1;
		}
		bins[q] = bin;
		*index += 1;
		bin = bin->next;
	}
}

static size_t	count(t_bin *src)
{
	t_bin	*bin;
	size_t	bin_num;

	bin = src;
	bin_num = 0;
	while (bin != NULL)
	{
		bin_num += 1;
		bin = bin->next;
	}
	return (bin_num);
}

void			last_block(size_t gd_tot, char *buffer, size_t *offset)
{
	buff_string("\n\x1b[1;31mTotal (all): \x1b[0m", buffer, offset);
	buff_number(10, gd_tot, buffer, offset);
	buff_string(" bytes\n", buffer, offset);
	flush_buffer(buffer, offset);
}

t_bin			**part2(t_arena *arena, char *buffer, size_t *offset)
{
	size_t	bin_num;
	t_bin	**bins;
	size_t	index;

	bin_num = count(arena->tiny) + count(arena->small) + count(arena->large);
	bins = (t_bin **)calloc(bin_num, sizeof(t_bin *));
	if (bins == NULL)
	{
		buff_string("show_alloc_mem: error allocating\n", buffer, offset);
		flush_buffer(buffer, offset);
		abort();
	}
	index = 0;
	step(arena->tiny, bins, &index);
	step(arena->small, bins, &index);
	step(arena->large, bins, &index);
	return (bins);
}

void			print_part1(t_arena *arena, char *buffer, size_t *offset, int k)
{
	buff_string("\x1b[33mARENA AT ", buffer, offset);
	buff_number(18, (unsigned long)arena, buffer, offset);
	buff_string(" (", buffer, offset);
	buff_number(10, (unsigned int)(k + 1), buffer, offset);
	buff_string(")\x1b[0m\n", buffer, offset);
}
