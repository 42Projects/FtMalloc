#include "malloc.h"
#include "arenap.h"


static int
test_chunk (t_bin *bin, t_chunk *chunk) {

	t_chunk *check = bin->chunk;
	while (check != __mbin_end(bin)) {

		if (check == chunk) return 0;

		check = __mchunk_next(check);
	}

	return 2;
}

static int
test_bin (t_bin *bin, t_chunk *chunk, t_bin **lock_bin) {

	while (bin != NULL) {
		if ((size_t)chunk > (size_t)bin && (size_t)chunk < (size_t)__mbin_end(bin)) {
			*lock_bin = bin;

			if (chunk->used == 0) return 2;

			return test_chunk(bin, chunk);
		}
		bin = bin->next;
	}

	return 1;
}

int
test_valid_chunk (t_chunk *chunk, t_bin **bin) {

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if (g_arena_data == NULL || (unsigned long)chunk % 16UL != 0) return 1;

	for (int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];
		pthread_mutex_lock(&arena->mutex);

		int ret = test_bin(arena->tiny, chunk, bin);
		if (ret != 1) {
			pthread_mutex_unlock(&arena->mutex);
			return (ret == 2) ? 1 : 0;
		}

		ret = test_bin(arena->small, chunk, bin);
		if (ret != 1) {
			pthread_mutex_unlock(&arena->mutex);
			return (ret == 2) ? 1 : 0;
		}

		ret = test_bin(arena->large, chunk, bin);
		if (ret != 1) {
			pthread_mutex_unlock(&arena->mutex);
			return (ret == 2) ? 1 : 0;
		}

		pthread_mutex_unlock(&arena->mutex);
	}

	return 1;
}

void
remove_chunk (t_bin *bin, t_chunk *chunk) {

	/* Return memory chunk to the bin and update bin free size. */
	bin->free_size += chunk->size;

	/* If the bin is empty, clean it */
	if (bin->free_size + sizeof(t_bin) == bin->size) {

		t_bin *main_bin = __mbin_get_main(bin->arena, bin->type);
		if ((__builtin_expect(g_arena_data->env & M_RELEASE_BIN, 0)) && main_bin != bin) {

			bin->prev->next = bin->next;
			if (bin->next != NULL) bin->next->prev = bin->prev;

			munmap(bin, bin->size);
			return;

		} else {
			chunk = bin->chunk;
			chunk->size = bin->free_size;
			chunk->used = 0;
			bin->max_chunk_size = chunk->size;

			if (bin->type != CHUNK_LARGE) __marena_update_max_chunks(bin, 0);
		}

	} else {
		chunk->used = 0;

		/* Defragment. */
		t_chunk *next_chunk = __mchunk_next(chunk);
		if (next_chunk != __mbin_end(bin) && next_chunk->used == 0) chunk->size += next_chunk->size;

		next_chunk = __mchunk_next(chunk);
		if (next_chunk != __mbin_end(bin)) next_chunk->prev = chunk;

		if (chunk->size > bin->max_chunk_size) {
			bin->max_chunk_size = chunk->size;
			__marena_update_max_chunks(bin, 0);
		}
	}

	if (__builtin_expect(g_arena_data->env & M_SCRIBBLE, 0)) {
		ft_memset(chunk->user_area, 0x55, chunk->size - sizeof(t_chunk));
	}
}

void
free (void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *)ptr - 1;
	t_bin *bin = NULL;
	if (__builtin_expect(test_valid_chunk(chunk, &bin) != 0, 0)) {
		if (g_arena_data->env & M_ABORT_ON_ERROR) {
			(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) << 1);
			abort();
		}

		return;
	}

	t_arena *arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);
	remove_chunk(bin, chunk);
	pthread_mutex_unlock(&arena->mutex);
}
