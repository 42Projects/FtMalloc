#include "arenap.h"
#include "freep.h"


void
__free (void *ptr) {

	if (ptr == NULL) return;

	t_chunk *chunk = (t_chunk *)((unsigned long)ptr - sizeof(t_chunk));

	/* If the pointer is not aligned on a 16bytes boundary, it is invalid by definition. */
	if ((unsigned long)chunk % 16UL != 0 || __mchunk_is_used(chunk) == 0) {
		(void)(write(STDERR_FILENO, "free(): invalid pointer\n", 24) + 1);
		abort();
	}

	t_bin *bin = chunk->bin;
	t_arena *arena = bin->arena;
	pthread_mutex_lock(&arena->mutex);

	/* We return the memory space to the bin free size. If the bin is empty, we unmap it. */
	bin->free_size += __mchunk_size(chunk);
	if (bin->free_size + sizeof(t_bin) == __mbin_size(bin)) {

		if (__mbin_main(bin)) {

			chunk = bin->chunk;
			chunk->size = bin->free_size;
			bin->max_chunk_size = chunk->size;

			if  (__mchunk_type_nomatch(bin, CHUNK_LARGE)) __marena_update_max_chunks(bin, 0);

			memset(chunk->user_area, 0, chunk->size - sizeof(t_chunk));

		} else {

			if (bin->left != NULL) bin->left->right = bin->right;
			if (bin->right != NULL) bin->right->left = bin->left;

			munmap(bin, bin->size & SIZE_MASK);
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

	pthread_mutex_unlock(&arena->mutex);
}
