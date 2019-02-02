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
buff_number (int base, __uint64_t number, char *buffer, size_t *offset) {
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

	if (g_main_arena == NULL) return;

	char 		buffer[BUFF_SIZE];
	size_t 		offset = 0;
	t_arena 	*arena = g_main_arena;
	t_pool 		*pool = NULL;
	t_chunk		*chunk = NULL;

	do {
		buff_string("\n", buffer, &offset);

		/* Display arena address. */
		buff_string("ARENA AT ", buffer, &offset);
		buff_number(16, (__uint64_t)arena, buffer, &offset);

		if (arena == g_main_arena) buff_string(" (MAIN)", buffer, &offset);

		buff_string("\n", buffer, &offset);
		pool = (t_pool *)arena->pool;

		do {

			if (pool_type_match(pool, CHUNK_TYPE_TINY) != 0) {
				buff_string("TINY : ", buffer, &offset);
			} else if (pool_type_match(pool, CHUNK_TYPE_SMALL) != 0) {
				buff_string("SMALL : ", buffer, &offset);
			} else {
				buff_string("LARGE : ", buffer, &offset);
			}

			buff_number(16, (__uint64_t)pool, buffer, &offset);
			buff_string("\n", buffer, &offset);

			chunk = (t_chunk *)pool->chunk;

			do {
				buff_number(16, (__uint64_t)chunk, buffer, &offset);
				buff_string(" - ", buffer, &offset);
				buff_number(16, (__uint64_t)(chunk + chunk->size), buffer, &offset);
				buff_string(" : ", buffer, &offset);
				buff_number(10, chunk->size, buffer, &offset);
				buff_string(" bytes\n", buffer, &offset);

				chunk = chunk->next;
			} while (chunk != NULL);

			pool = pool->next;
		} while (pool != NULL);

		arena = arena->next;
	} while (arena != NULL && arena != g_main_arena);

	flush_buffer(buffer, &offset);
};



void
show_alloc_mem_ex (void) {

	if (g_main_arena == NULL) return;

};