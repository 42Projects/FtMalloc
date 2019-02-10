#include "showp.h"


static inline void
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
}

static void
buff_number (int base, unsigned long number, char *buffer, size_t *offset) {

	const char dec[10] = "0123456789";
	const char hexa[16] = "0123456789abcdef";
	char rev_buff[16];

	if (number == 0) buffer[(*offset)++] = '0';

	int k = 0;
	while (number != 0) {
		rev_buff[k++] = (base == 10) ? dec[number % 10] : hexa[number % 16];
		number /= (base == 10) ? 10 : 16;
	}

	if (*offset >= BUFF_SIZE - 19) flush_buffer(buffer, offset);

	if (base == 16) {
		buffer[(*offset)++] = '0';
		buffer[(*offset)++] = 'x';
	}

	while (--k >= 0) buffer[(*offset)++] = rev_buff[k];

	if (*offset == BUFF_SIZE - 1) flush_buffer(buffer, offset);
}

static inline void
explore_bin (t_bin *bin, int chunk_type, char *buffer, size_t *offset, size_t *arena_total) {

	while (bin != NULL) {

		if (chunk_type == CHUNK_LARGE) {
			buff_string("\x1b[36mLARGE (SIZE ", buffer, offset);
			buff_number(10, (unsigned long)__mbin_end(bin) - (unsigned long)bin, buffer, offset);
			buff_string("):\x1b[0m ", buffer, offset);
		} else if (chunk_type == CHUNK_TINY) {
			buff_string("\x1b[36mTINY :\x1b[0m ", buffer, offset);
		} else if (chunk_type == CHUNK_SMALL) {
			buff_string("\x1b[36mSMALL :\x1b[0m ", buffer, offset);
		}

		buff_number(16, (unsigned long)bin, buffer, offset);
		buff_string("\n", buffer, offset);

		t_chunk *chunk = bin->chunk;
		while (chunk != __mbin_end(bin)) {

			if (__mchunk_is_used(chunk)) {
				size_t chunk_size = __mchunk_size(chunk) - sizeof(t_chunk);

				buff_number(16, (unsigned long)chunk->user_area, buffer, offset);
				buff_string(" - ", buffer, offset);
				buff_number(16, (unsigned long)__mchunk_next(chunk), buffer, offset);
				buff_string(" : ", buffer, offset);
				buff_number(10, chunk_size, buffer, offset);
				buff_string(" bytes\n", buffer, offset);

				*arena_total += chunk_size;
			}

			chunk = __mchunk_next(chunk);
		}

		bin = (chunk_type == CHUNK_TINY) ? bin->left : bin->right;
	}
}

void
show_alloc_mem (void) {

	char		 			buffer[BUFF_SIZE];
	size_t 					offset = 0;

	if (g_arena_data == NULL) return;

	for (int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];
		size_t arena_total = 0;

		buff_string("\n", buffer, &offset);

		/* Display arena address. */
		buff_string("\x1b[31mARENA AT ", buffer, &offset);
		buff_number(16, (unsigned long)arena, buffer, &offset);

		if (k == 0) buff_string(" (MAIN)", buffer, &offset);

		buff_string("\x1b[0m\n", buffer, &offset);

		t_bin *bin = arena->small_bins;
		if (bin != NULL && __mchunk_type_match(bin, CHUNK_SMALL)) bin = bin->left;
		explore_bin(bin, CHUNK_TINY, buffer, &offset, &arena_total);

		bin = arena->small_bins;
		if (bin != NULL && __mchunk_type_match(bin, CHUNK_TINY)) bin = bin->right;
		explore_bin(bin, CHUNK_SMALL, buffer, &offset, &arena_total);

		bin = arena->large_bins;
		explore_bin(bin, CHUNK_LARGE, buffer, &offset, &arena_total);

		buff_string("\x1b[31mTotal: \x1b[0m", buffer, &offset);
		buff_number(10, arena_total, buffer, &offset);
		buff_string(" bytes\n", buffer, &offset);
	}

	flush_buffer(buffer, &offset);
};

void
show_alloc_mem_ex (void) {

	if (g_arena_data == NULL) return;

};