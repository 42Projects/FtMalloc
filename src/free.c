#include "malloc.h"
#include "arenap.h"

#define BUFF_SIZE 4096
static void
flush_buffer (char *buffer, size_t *offset) {

	buffer[*offset] = '\0';
	(void)(write(1, buffer, *offset) + 1);
	*offset = 0;
}

static void
buff_string (const char *s, char *buffer, size_t *offset) {

	while (*s) {
		buffer[(*offset)++] = *s++;

		if (*offset == BUFF_SIZE - 1) flush_buffer(buffer, offset);
	}
	flush_buffer(buffer, offset);
}

static void
buff_number (int base, unsigned long number, char *buffer, size_t *offset) {

	const char dec[10] = "0123456789";
	const char hexa[16] = "0123456789abcdef";
	char rev_buff[16];

	if (number == 0) buffer[(*offset)++] = '0';
	if (base == 19 && number < 16) buffer[(*offset)++] = '0';

	int k = 0;
	while (number != 0) {
		rev_buff[k++] = (base == 10) ? dec[number % 10] : hexa[number % 16];
		number /= (base == 10) ? 10 : 16;
	}

	if (*offset >= BUFF_SIZE - 19) flush_buffer(buffer, offset);

	if (base == 18) {
		buffer[(*offset)++] = '0';
		buffer[(*offset)++] = 'x';
	}

	while (--k >= 0) buffer[(*offset)++] = rev_buff[k];

	if (base == 19) buffer[(*offset)++] = ' ';

	flush_buffer(buffer, offset);
}

static int
test_chunk (t_bin *bin, t_chunk *chunk, t_chunk **previous) {

	t_chunk *check = bin->chunk;

	while (check != __mbin_end(bin)) {

		if (check == chunk) return 0;

		*previous = check;
		check = __mchunk_next(check);
	}

	return 2;
}

static int
test_bin (t_bin *bin, t_chunk *chunk, int chunk_type, t_chunk **previous) {

	while (bin != NULL) {
		if ((size_t)chunk > (size_t)bin && (size_t)chunk < (size_t)__mbin_end(bin)) {

			if (__mchunk_not_used(chunk)) return 2;

			return test_chunk(bin, chunk, previous);
		}
		bin = (chunk_type == CHUNK_TINY) ? bin->left : bin->right;
	}

	return 1;
}

int
test_valid_chunk (t_chunk *chunk, t_chunk **previous) {

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if (g_arena_data == NULL || (unsigned long)chunk % 16UL != 0) return 1;

	for (int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];
		pthread_mutex_lock(&arena->mutex);

		int ret = test_bin(arena->small_bins, chunk, CHUNK_TINY, previous);
		if (ret != 1) {
			pthread_mutex_unlock(&arena->mutex);
			return (ret == 2) ? 1 : 0;
		}

		ret = test_bin(arena->small_bins, chunk, CHUNK_SMALL, previous);
		if (ret != 1) {
			pthread_mutex_unlock(&arena->mutex);
			return (ret == 2) ? 1 : 0;
		}

		ret = test_bin(arena->large_bins, chunk, CHUNK_LARGE, previous);
		if (ret != 1) {
			pthread_mutex_unlock(&arena->mutex);
			return (ret == 2) ? 1 : 0;
		}
		pthread_mutex_unlock(&arena->mutex);
	}

	return 1;
}

void
remove_chunk (t_bin *bin, t_chunk *chunk, t_chunk *previous) {

	/* Return memory chunk to the bin and update bin free size. */
	bin->free_size += __mchunk_size(chunk);

	/* If the bin is empty, clean it */
	if (bin->free_size + sizeof(t_bin) == __mbin_size(bin)) {
		chunk = bin->chunk;
		chunk->size = bin->free_size;
		bin->max_chunk_size = chunk->size;

		if  (__mbin_type_not(bin, CHUNK_LARGE)) __marena_update_max_chunks(bin, 0);

	} else {

		chunk->size &= ~(1UL << CHUNK_USED);

		/* Defragment memory. */
		t_chunk *next_chunk = __mchunk_next(chunk);
		if (next_chunk != __mbin_end(bin) && __mchunk_not_used(next_chunk)) chunk->size += next_chunk->size;

		if (previous != NULL && __mchunk_not_used(previous)) {
			previous->size += chunk->size;
			chunk = previous;
		}

		if (chunk->size > bin->max_chunk_size) {
			bin->max_chunk_size = chunk->size;
			__marena_update_max_chunks(bin, 0);
		}
	}

	/* Clear user area from garbage data. */
	memset(chunk->user_area, 0, chunk->size - sizeof(t_chunk));
}

void
free(void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *)ptr - 1, *previous = NULL;
	if (__builtin_expect(test_valid_chunk(chunk, &previous) != 0, 0)) {
		if (g_arena_data->env & M_ABORT_ON_ERROR) {
			(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) << 1);
			abort();
		}

		return;
	}

	t_bin *bin = chunk->bin;
	t_arena *arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);
	remove_chunk(bin, chunk, previous);
	pthread_mutex_unlock(&arena->mutex);
}
