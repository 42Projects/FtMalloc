/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:28:44 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "showp.h"
#include <unistd.h>

#define DEC "0123456789"
#define HEXA "0123456789abcdef"

void	flush_buffer(char *buffer, size_t *offset)
{
	buffer[*offset] = '\0';
	(void)(write(1, buffer, *offset) + 1);
	*offset = 0;
}

void	buff_string(const char *s, char *buffer, size_t *offset)
{
	while (*s)
	{
		buffer[(*offset)++] = *s++;
		if (*offset == BUFF_SIZE - 1)
			flush_buffer(buffer, offset);
	}
	flush_buffer(buffer, offset);
}

void	buff_number(int base, size_t number, char *buffer, size_t *offset)
{
	char			rev_buff[16];
	int				k;

	k = 0;
	if (number == 0)
		buffer[(*offset)++] = '0';
	if (base == 19 && number < 16)
		buffer[(*offset)++] = '0';
	while (number != 0)
	{
		rev_buff[k++] = (base == 10) ? DEC[number % 10] : HEXA[number % 16];
		number /= (base == 10) ? 10 : 16;
	}
	if (*offset >= BUFF_SIZE - 19)
		flush_buffer(buffer, offset);
	if (base == 18)
	{
		buffer[(*offset)++] = '0';
		buffer[(*offset)++] = 'x';
	}
	while (--k >= 0)
		buffer[(*offset)++] = rev_buff[k];
	if (base == 19)
		buffer[(*offset)++] = ' ';
	flush_buffer(buffer, offset);
}
