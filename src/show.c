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



void
show_alloc_mem (void) {

	if (g_arena_data == NULL) return;

	char		 		buffer[BUFF_SIZE];
	size_t 				offset = 0;
	t_pool 				*pool = NULL;
	t_alloc_chunk		*chunk = NULL;

	for (int k = 0; k < g_arena_data->arena_count; k++) {
		t_arena *arena = &g_arena_data->arenas[k];

		buff_string("\n", buffer, &offset);

		/* Display arena address. */
		buff_string("\x1b[31mARENA AT ", buffer, &offset);
		buff_number(16, (unsigned long)arena, buffer, &offset);

		if (k == 0) buff_string(" (MAIN)", buffer, &offset);

		buff_string("\x1b[0m\n", buffer, &offset);

		pool = arena->main_pool;
		if (pool_type_match(pool, CHUNK_TYPE_LARGE)) pool = pool->right;

		while (pool != NULL) {

			if (pool_type_match(pool, CHUNK_TYPE_TINY)) {
				buff_string("\x1b[36mTINY :\x1b[0m ", buffer, &offset);
			} else if (pool_type_match(pool, CHUNK_TYPE_SMALL)) {
				buff_string("\x1b[36mSMALL :\x1b[0m ", buffer, &offset);
			}

			buff_number(16, (unsigned long)pool, buffer, &offset);
			buff_string("\n", buffer, &offset);

			chunk = (t_alloc_chunk *)pool->chunk;

			while (1) {
				buff_number(16, (unsigned long)chunk->user_area, buffer, &offset);
				buff_string(" - ", buffer, &offset);
				buff_number(16, (unsigned long)chunk + chunk->size, buffer, &offset);
				buff_string(" : ", buffer, &offset);
				buff_number(10, chunk->size - sizeof(t_alloc_chunk), buffer, &offset);
				buff_string(" bytes\n", buffer, &offset);

				chunk = (t_alloc_chunk *)((unsigned long)chunk + chunk->size);

				if (chunk_is_allocated(chunk) == 0) break;
			}

			pool = pool->right;
		}

		pool = arena->main_pool;
		if (pool_type_match(pool, CHUNK_TYPE_LARGE) == 0) pool = pool->left;

		while (pool != NULL) {

			buff_string("\x1b[36mLARGE :\x1b[0m ", buffer, &offset);
			buff_number(16, (unsigned long)pool, buffer, &offset);
			buff_string("\n", buffer, &offset);
			chunk = (t_alloc_chunk *)pool->chunk;
			buff_number(16, (unsigned long)chunk->user_area, buffer, &offset);
			buff_string(" - ", buffer, &offset);
			buff_number(16, (unsigned long)chunk + chunk->size, buffer, &offset);
			buff_string(" : ", buffer, &offset);
			buff_number(10, chunk->size - sizeof(t_alloc_chunk), buffer, &offset);
			buff_string(" bytes\n", buffer, &offset);

			pool = pool->left;
		}
	}

	flush_buffer(buffer, &offset);

};



void
show_alloc_mem_ex (void) {

	if (g_arena_data == NULL) return;

};