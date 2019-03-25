/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:28:14 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ARENAP_H
# define ARENAP_H

# include <errno.h>
# include <pthread.h>
# include <sys/mman.h>

/*
** For memset.
*/
# include <string.h>

/*
** For abort.
*/
# include <stdlib.h>

# define M_ARENA_MAX 8
# define CHUNKS_PER_POOL 100
# define SIZE_TINY 1024
# define SIZE_SMALL 4096
# define SIZE_THRESHOLD ((1UL << 62) - 16)
# define ATM _Atomic int

enum						e_type
{
	CHUNK_TINY,
	CHUNK_SMALL,
	CHUNK_LARGE
};

enum						e_env
{
	M_ABORT_ON_ERROR = 1,
	M_RELEASE_BIN = 2,
	M_SCRIBBLE = 4,
	M_SHOW_HEXDUMP = 8,
	M_SHOW_UNALLOCATED = 16,
	M_SHOW_DEBUG = 32
};

typedef struct s_chunk		t_chunk;
typedef struct s_bin		t_bin;
typedef struct s_arena		t_arena;
typedef struct s_arena_data	t_arena_data;

struct						s_chunk
{
	unsigned long	size: 63;
	unsigned long	used: 1;
	struct s_chunk	*prev;
	void			*user_area[];
};

struct						s_bin
{
	unsigned long	free_size;
	unsigned long	size: 62;
	unsigned long	type: 2;
	unsigned long	max_chunk_size;
	struct s_arena	*arena;
	struct s_bin	*next;
	struct s_bin	*prev;
	t_chunk			chunk[];
};

struct						s_arena
{
	pthread_mutex_t	mutex;
	t_bin			*tiny;
	t_bin			*small;
	t_bin			*large;
	unsigned long	max_chunk_tiny;
	unsigned long	max_chunk_small;
};

struct						s_arena_data
{
	ATM				arena_count;
	t_arena			arenas[M_ARENA_MAX];
	int				env;
	unsigned long	pagesize;
};

# define MBIN_END(bin) ((void *)((unsigned long)bin + bin->size))
# define MCHUNK_NEXT(chunk) ((t_chunk *)((unsigned long)chunk + chunk->size))

extern t_arena_data			*g_arena_data;

void						ft_memset(void *b, int c, size_t len);
void						remove_chunk(t_bin *bin, t_chunk *chunk);
void						arena_update_max_chunks(t_bin *bin,
	unsigned long old_size);
int							test_valid_chunk(t_chunk *chunk, t_bin **bin);
t_bin						*bin_get_main(t_arena *arena,
	unsigned long chunk_type);

#endif
