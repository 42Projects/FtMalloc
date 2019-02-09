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
#include <stdio.h>
void
show_alloc_mem (void) {

	char		 			buffer[BUFF_SIZE];
	size_t 					offset = 0;
	t_pool 					*pool = NULL;
	t_chunk					*chunk = NULL;


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

		pool = (__mchunk_type_match(arena->main_pool, CHUNK_LARGE)) ? arena->main_pool->right : arena->main_pool;
		while (pool != NULL) {

			if (__mchunk_type_match(pool, CHUNK_TINY)) {
				buff_string("\x1b[36mTINY :\x1b[0m ", buffer, &offset);
			} else if (__mchunk_type_match(pool, CHUNK_SMALL)) {
				buff_string("\x1b[36mSMALL :\x1b[0m ", buffer, &offset);
			}

			buff_number(16, (unsigned long)pool, buffer, &offset);
			buff_string("\n", buffer, &offset);

			chunk = pool->chunk;
			while (chunk != __mpool_end(pool)) {

				if (__mchunk_is_used(chunk)) {
					size_t chunk_size = chunk->size - sizeof(t_chunk);

					buff_number(16, (unsigned long)chunk->user_area, buffer, &offset);
					buff_string(" - ", buffer, &offset);
					buff_number(16, (unsigned long)__mchunk_next(chunk), buffer, &offset);
					buff_string(" : ", buffer, &offset);
					buff_number(10, chunk_size, buffer, &offset);
					buff_string(" bytes\n", buffer, &offset);

					arena_total += chunk_size;
				}

				chunk = __mchunk_next(chunk);
			}

			pool = pool->right;
		}

		pool = (__mchunk_type_match(arena->main_pool, CHUNK_LARGE)) ? arena->main_pool : arena->main_pool->left;
		while (pool != NULL) {

			buff_string("\x1b[36mLARGE :\x1b[0m ", buffer, &offset);
			buff_number(16, (unsigned long) pool, buffer, &offset);
			buff_string("\n", buffer, &offset);

			chunk = pool->chunk;
			if (__mchunk_is_used(chunk)) {
				size_t chunk_size = chunk->size - sizeof(t_chunk);

				buff_number(16, (unsigned long) chunk->user_area, buffer, &offset);
				buff_string(" - ", buffer, &offset);
				buff_number(16, (unsigned long) __mchunk_next(chunk), buffer, &offset);
				buff_string(" : ", buffer, &offset);
				buff_number(10, chunk_size, buffer, &offset);
				buff_string(" bytes\n", buffer, &offset);

				arena_total += chunk_size;
			}

			pool = pool->left;
		}

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