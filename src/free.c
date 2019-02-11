#include "malloc.h"
#include "arenap.h"


void
remove_chunk (t_arena *arena, t_bin *bin, t_chunk *chunk) {

	/*
	   Return memory chunk to the bin and update bin free size. If the bin is empty, unmap it.
	   We never unmap the main bin to prevent too many syscalls.
	*/

	bin->free_size += __mchunk_size(chunk);
	if (bin->free_size + sizeof(t_bin) == __mbin_size(bin)) {

		if (bin == __mbin_main(bin)) {

			chunk = bin->chunk;
			chunk->size = bin->free_size;
			bin->max_chunk_size = chunk->size;

			if  (__mbin_type_not(bin, CHUNK_LARGE)) __marena_update_max_chunks(bin, 0);

			memset(chunk->user_area, 0, chunk->size - sizeof(t_chunk));

		} else {

			if (bin->left != NULL) bin->left->right = bin->right;
			if (bin->right != NULL) bin->right->left = bin->left;

			munmap(bin, bin->size & SIZE_MASK);
			return;
		}

	} else {
		chunk->size &= ~(1UL << CHUNK_USED);

		/* Defragment memory. */
		t_chunk *next_chunk = __mchunk_next(chunk);
		if (next_chunk != __mbin_end(bin) && __mchunk_not_used(next_chunk)) {
			chunk->size += next_chunk->size;
			memset(next_chunk, 0, sizeof(t_chunk));
		}

		if (__mchunk_size(chunk) > bin->max_chunk_size) {
			bin->max_chunk_size = __mchunk_size(chunk);
			__marena_update_max_chunks(bin, 0);
		}
	}

	/* If the new max chunk size of the bin is larger than that of the first bin in the list, reorder the list. */
	t_bin *first_bin = __mbin_main(bin);
	if (__mbin_type_is(bin, CHUNK_TINY)) {
		first_bin = first_bin->left;
	} else {
		first_bin = first_bin->right;
	}

	if (first_bin != NULL && bin != first_bin && bin->max_chunk_size > first_bin->max_chunk_size) {
		t_bin *main_bin = __mbin_main(bin);

		if (bin->left != NULL) bin->left->right = bin->right;
		if (bin->right != NULL) bin->right->left = bin->left;

		/* Insert the new bin into the bin list. */
		if (__mbin_type_is(bin, CHUNK_TINY)) {
			main_bin->left = bin;
			bin->left = first_bin;
			bin->right = main_bin;
			first_bin->right = bin;

		} else {
			main_bin->right = bin;
			bin->right = first_bin;
			bin->left = main_bin;
			first_bin->left = bin;

		}
	}

}

void
free(void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *)ptr - 1;

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if (g_arena_data == NULL || (unsigned long)chunk % 16UL != 0 || __mchunk_invalid(chunk)) {
		(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
		abort();
	}

	t_bin *bin = chunk->bin;
	t_arena *arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);
	remove_chunk(arena, bin, chunk);
	pthread_mutex_unlock(&arena->mutex);
}
