#include "showp.h"



static inline void
flush_buffer (char *buffer, size_t *offset) {
	buffer[*offset] = '\0';
	(void)(write(1, buffer, *offset) + 1);
	*offset = 0;
}

static void
buff_string(const char *s, char *buffer, size_t *offset) {
	while (*s) {
		buffer[(*offset)++] = *s++;
		if (*offset == BUFF_SIZE - 1) flush_buffer(buffer, offset);
	}
}

static void
buff_address (const void *ptr, char *buffer, size_t *offset) {
	const char hexa[16] = "0123456789abcdef";
	char rev_buff[16];
	unsigned long address;

	address = (unsigned long)ptr;

	int k = 0;
	while (address != 0) {
		rev_buff[k++] = hexa[address % 16];
		address /= 16;
	}

	if (*offset >= BUFF_SIZE - 19) flush_buffer(buffer, offset);

	buffer[(*offset)++] = '0';
	buffer[(*offset)++] = 'x';

	while (--k >= 0) buffer[(*offset)++] = rev_buff[k];

	if (*offset == BUFF_SIZE - 1) flush_buffer(buffer, offset);
}



void
show_alloc_mem (void) {

	if (g_main_arena == NULL) return;

	char buffer[BUFF_SIZE];
	size_t offset = 0;
	t_arena *arena = g_main_arena;

	do {
		if (is_main_arena(arena) != 0) buff_string("MAIN ", buffer, &offset);

		buff_string("ARENA AT ", buffer, &offset);
		buff_address((void *) arena, buffer, &offset);
		buff_string(":\n", buffer, &offset);

		arena = arena->next;
	} while (arena != NULL && is_main_arena(arena) == 0);

	flush_buffer(buffer, &offset);
};



void
show_alloc_mem_ex (void) {

	if (g_main_arena == NULL) return;

};