/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:12 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "showp.h"

static void	cut1(unsigned char *ptr, char *buffer, size_t *offset, int k)
{
	buff_number(19, (unsigned long)(ptr[k]), buffer, offset);
	if (k == 7)
		buffer[(*offset)++] = ' ';
}

static void	cut2(char *ptr, char *buffer, size_t *offset, int k)
{
	char	c;

	c = ptr[k];
	buffer[(*offset)++] = (c > 31 && c < 127) ? c : (char)32;
}

void		hexdump_chunk(t_chunk *chunk, char *buffer, size_t *offset)
{
	void	*end;
	t_chunk	*ptr;
	int		k;

	end = MCHUNK_NEXT(chunk);
	ptr = chunk;
	while ((unsigned long)ptr != (unsigned long)end)
	{
		if (ptr == chunk)
			buff_string("\x1b[1;31m", buffer, offset);
		buff_number(16, (unsigned long)ptr, buffer, offset);
		buff_string("  ", buffer, offset);
		k = -1;
		while (++k < 16)
			cut1((unsigned char *)ptr, buffer, offset, k);
		buff_string(" |", buffer, offset);
		k = -1;
		while (++k < 16)
			cut2((char*)ptr, buffer, offset, k);
		buff_string("|\n", buffer, offset);
		if (ptr == chunk)
			buff_string("\x1b[0m", buffer, offset);
		++ptr;
	}
}
