#include "showp.h"


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

static void
hexdump_chunk (t_chunk *chunk, char *buffer, size_t *offset) {

	void *end = __mchunk_next(chunk);
	t_chunk *ptr = chunk;

	while ((unsigned long)ptr != (unsigned long)end) {

		if (ptr == chunk) buff_string("\x1b[1;31m", buffer, offset);

		buff_number(16, (unsigned long)ptr, buffer, offset);
		buff_string("  ", buffer, offset);

		for (int k = 0; k < 16; k++) {
			buff_number(19, (unsigned long)(((unsigned char *)ptr)[k]), buffer, offset);

			if (k == 7) buffer[(*offset)++] = ' ';
		}

		buff_string(" |", buffer, offset);

		for (int k = 0; k < 16; k++) {
			char c = ((char *)ptr)[k];
			buffer[(*offset)++] = (c > 31 && c < 127) ? c : (char)32;
		}

		buff_string("|\n", buffer, offset);

		if (ptr == chunk) buff_string("\x1b[0m", buffer, offset);

		++ptr;
	}
}

static void
explore_bins (t_bin *bins[], size_t bin_num, char *buffer, size_t *offset, size_t *arena_total) {

	for (size_t k = 0; k < bin_num; k++) {
		t_bin *bin = bins[k];
		if (__mbin_type_is(bin, CHUNK_TINY)) {
			buff_string("\x1b[36mTINY :  ", buffer, offset);
		} else if (__mbin_type_is(bin, CHUNK_SMALL)) {
			buff_string("\x1b[36mSMALL : ", buffer, offset);
		} else {
			buff_string("\x1b[36mLARGE : ", buffer, offset);
		}

		buff_number(18, (unsigned long)bin, buffer, offset);
		buff_string("\x1b[0m", buffer, offset);
		if (g_arena_data->env & M_SHOW_DEBUG) {
			buff_string(" [size = ", buffer, offset);
			buff_number(10, __mbin_size(bin), buffer, offset);
			buff_string("], [free size = ", buffer, offset);
			buff_number(10, bin->free_size, buffer, offset);
			buff_string("], [max chunk size = ", buffer, offset);
			buff_number(10, bin->max_chunk_size, buffer, offset);
			buffer[(*offset)++] = ']';
		}
		buffer[(*offset)++] = '\n';

		t_chunk *chunk = bin->chunk;
		while (chunk != __mbin_end(bin)) {
			if (chunk->used || g_arena_data->env & M_SHOW_UNALLOCATED) {
				size_t chunk_size = chunk->size - sizeof(t_chunk);

				if ((chunk->used == 0 && g_arena_data->env & M_SHOW_UNALLOCATED)
					|| (g_arena_data->env & M_SHOW_HEXDUMP) == 0) {
					buff_number(18, (unsigned long)chunk->user_area, buffer, offset);
					buff_string(" - ", buffer, offset);
					buff_number(18, (unsigned long)__mchunk_next(chunk), buffer, offset);
					buff_string(" : ", buffer, offset);
					buff_number(10, chunk_size, buffer, offset);
					buff_string(" bytes", buffer, offset);

					if (chunk->used == 0) buff_string(" \x1b[92m(UNALLOCATED)\x1b[0m", buffer, offset);

					buffer[(*offset)++] = '\n';
				}

				if (chunk->used) {

					if (g_arena_data->env & M_SHOW_HEXDUMP) hexdump_chunk(chunk, buffer, offset);

					*arena_total += chunk_size;
				}
			}
			chunk = __mchunk_next(chunk);
		}
	}
}

void
show_alloc_mem (void) {

	char		 			buffer[BUFF_SIZE];
	size_t 					offset = 0,
							grand_total = 0;

	if (g_arena_data == NULL) return;

	for (int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];
		pthread_mutex_lock(&arena->mutex);
		size_t arena_total = 0;

		if (k != 0) buffer[offset++] = '\n';

		buff_string("\x1b[33mARENA AT ", buffer, &offset);
		buff_number(18, (unsigned long)arena, buffer, &offset);
		buff_string(" (", buffer, &offset);
		buff_number(10, (unsigned int)(k + 1), buffer, &offset);
		buff_string(")\x1b[0m\n", buffer, &offset);

		size_t bin_num = 0;
		for (int p = 0; p < 3; p++) {
			t_bin *bin = (p == 0) ? arena->tiny : (p == 1) ? arena->small : arena->large;
			while (bin != NULL) {
				bin_num += 1;
				bin = bin->next;
			}
		}

		t_bin **bins = (t_bin **)calloc(bin_num, sizeof(t_bin *));
		if (bins == NULL) {
			buff_string("show_alloc_mem: error allocating\n", buffer, &offset);
			flush_buffer(buffer, &offset);
			abort();
		}

		size_t index = 0;
		for (int p = 0; p < 3; p++) {
			t_bin *bin = (p == 0) ? arena->tiny : (p == 1) ? arena->small : arena->large;
			while (bin != NULL) {
				size_t q = 0;
				while (q < index) {
					if (bins[q] > bin) {
						for (size_t r = index; r > q; r--) {
							bins[r] = bins[r - 1];
						}
						break;
					}
					q += 1;
				}
				bins[q] = bin;
				index += 1;
				bin = bin->next;
			}
		}

		explore_bins(bins, bin_num, buffer, &offset, &arena_total);
		buff_string("\x1b[33mTotal (arena): \x1b[0m", buffer, &offset);
		buff_number(10, arena_total, buffer, &offset);
		buff_string(" bytes\n", buffer, &offset);
		grand_total += arena_total;
		pthread_mutex_unlock(&arena->mutex);
	}

	buff_string("\n\x1b[1;31mTotal (all): \x1b[0m", buffer, &offset);
	buff_number(10, grand_total, buffer, &offset);
	buff_string(" bytes\n", buffer, &offset);
	flush_buffer(buffer, &offset);
};
