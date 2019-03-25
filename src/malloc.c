/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:29:15 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include "malloc_tools.h"

t_arena_data	*g_arena_data = NULL;

static void		my_setenv(void)
{
	char *val;

	val = NULL;
	if ((val = getenv("M_ABORT_ON_ERROR")) != NULL && val[0] == '1')
		g_arena_data->env |= M_ABORT_ON_ERROR;
	if ((val = getenv("M_RELEASE_BIN")) != NULL && val[0] == '1')
		g_arena_data->env |= M_RELEASE_BIN;
	if ((val = getenv("M_SCRIBBLE")) != NULL && val[0] == '1')
		g_arena_data->env |= M_SCRIBBLE;
	if ((val = getenv("M_SHOW_HEXDUMP")) != NULL && val[0] == '1')
		g_arena_data->env |= M_SHOW_HEXDUMP;
	if ((val = getenv("M_SHOW_UNALLOCATED")) != NULL && val[0] == '1')
		g_arena_data->env |= M_SHOW_UNALLOCATED;
	if ((val = getenv("M_SHOW_DEBUG")) != NULL && val[0] == '1')
		g_arena_data->env |= M_SHOW_DEBUG;
}

static void		*u_malloc(size_t size, int zero_set)
{
	static pthread_mutex_t	main_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	static pthread_mutex_t	new_arena_mutex = PTHREAD_MUTEX_INITIALIZER;
	static t_arena_data		arena_data = { .arena_count = 1, .env = 0 };
	size_t					chunk_type;

	size = (size + 0xfUL) & ~0xfUL;
	if (size > SIZE_THRESHOLD)
		return (NULL);
	else if (size > SIZE_SMALL)
		chunk_type = CHUNK_LARGE;
	else
		chunk_type = (size <= SIZE_TINY) ? CHUNK_TINY : CHUNK_SMALL;
	pthread_mutex_lock(&main_arena_mutex);
	if (__builtin_expect(g_arena_data == NULL, 0))
	{
		arena_data.pagesize = (size_t)sysconf(_SC_PAGESIZE);
		g_arena_data = &arena_data;
		my_setenv();
		return (first_cut(size, chunk_type, &main_arena_mutex, zero_set));
	}
	pthread_mutex_unlock(&main_arena_mutex);
	return (malloc_phase_2(&new_arena_mutex, size, chunk_type, zero_set));
}

void			*malloc(size_t size)
{
	return (u_malloc(size, 0));
}

void			*calloc(size_t nmemb, size_t size)
{
	return (u_malloc(nmemb * size, 1));
}
