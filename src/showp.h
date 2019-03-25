/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:39 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SHOWP_H
# define SHOWP_H

# include "arenap.h"

# define BUFF_SIZE 4096

void	print_part1(t_arena *arena, char *buffer, size_t *offset, int k);
t_bin	**part2(t_arena *arena, char *buffer, size_t *offset);
void	last_block(size_t gd_tot, char *buffer, size_t *offset);

void	flush_buffer(char *buffer, size_t *offset);
void	buff_string(const char *s, char *buffer, size_t *offset);
void	buff_number(int base, size_t number, char *buffer, size_t *offset);

void	hexdump_chunk(t_chunk *chunk, char *buffer, size_t *offset);

#endif
