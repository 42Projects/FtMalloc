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
			buffer[(*offset)++] = (c > 31 && c < 127) ? c : ' ';
		}

		buff_string("|\n", buffer, offset);

		if (ptr == chunk) buff_string("\x1b[0m", buffer, offset);

		++ptr;
	}
}

static void
explore_bin (t_arena *arena, int chunk_type, char *buffer, size_t *offset, size_t *arena_total) {

	t_bin *bin = __mbin_get_main(arena, chunk_type);
	while (bin != NULL) {

		if (chunk_type == CHUNK_LARGE) {
			buff_string("\x1b[36mLARGE : ", buffer, offset);
		} else if (chunk_type == CHUNK_TINY) {
			buff_string("\x1b[36mTINY : ", buffer, offset);
		} else if (chunk_type == CHUNK_SMALL) {
			buff_string("\x1b[36mSMALL : ", buffer, offset);
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
			if (__mchunk_is_used(chunk) || g_arena_data->env & M_SHOW_UNALLOCATED) {
				size_t chunk_size = __mchunk_size(chunk) - sizeof(t_chunk);

				if ((__mchunk_not_used(chunk) && g_arena_data->env & M_SHOW_UNALLOCATED)
					|| (g_arena_data->env & M_SHOW_HEXDUMP) == 0) {
					buff_number(18, (unsigned long)chunk->user_area, buffer, offset);
					buff_string(" - ", buffer, offset);
					buff_number(18, (unsigned long)__mchunk_next(chunk), buffer, offset);
					buff_string(" : ", buffer, offset);
					buff_number(10, chunk_size, buffer, offset);
					buff_string(" bytes", buffer, offset);

					if (__mchunk_not_used(chunk)) buff_string(" \x1b[92m(UNALLOCATED)\x1b[0m", buffer, offset);

					buffer[(*offset)++] = '\n';
				}

				if (__mchunk_is_used(chunk)) {

					if (g_arena_data->env & M_SHOW_HEXDUMP) hexdump_chunk(chunk, buffer, offset);

					*arena_total += chunk_size;
				}
			}
			chunk = __mchunk_next(chunk);
		}
		bin = bin->next;
	}
}

void
show_alloc_mem (void) {

	char		 			buffer[BUFF_SIZE];
	size_t 					offset = 0,
							grand_total = 0;

	if (g_arena_data == NULL) return;

	for (unsigned int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];
		size_t arena_total = 0;

		if (k != 0) buffer[offset++] = '\n';

		buff_string("\x1b[33mARENA AT ", buffer, &offset);
		buff_number(18, (unsigned long)arena, buffer, &offset);
		buff_string(" (", buffer, &offset);
		buff_number(10, k + 1, buffer, &offset);
		buff_string(")\x1b[0m\n", buffer, &offset);

		explore_bin(arena, CHUNK_TINY, buffer, &offset, &arena_total);
		explore_bin(arena, CHUNK_SMALL, buffer, &offset, &arena_total);
		explore_bin(arena, CHUNK_LARGE, buffer, &offset, &arena_total);

		buff_string("\x1b[33mTotal (arena): \x1b[0m", buffer, &offset);
		buff_number(10, arena_total, buffer, &offset);
		buff_string(" bytes\n", buffer, &offset);

		grand_total += arena_total;
	}

	buff_string("\n\x1b[1;31mTotal (all): \x1b[0m", buffer, &offset);
	buff_number(10, grand_total, buffer, &offset);
	buff_string(" bytes\n", buffer, &offset);

	flush_buffer(buffer, &offset);
};
