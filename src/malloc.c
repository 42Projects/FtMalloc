#include "malloc_private.h"
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <ompt.h>


static t_arena_map		*g_arena_map = NULL;
static pthread_mutex_t	g_arena_mutex = PTHREAD_MUTEX_INITIALIZER;


static void
terminate (void) {
	munmap(g_arena_map, g_arena_map->size);
}

static t_arena *
create_new_arena (size_t size, long pagesize, int flags, pthread_t self, pthread_mutex_t *mutex) {

	/* Our arena needs to contain the arena header and one header per chunk. Add these to the requested size. */
	static size_t headers_size = sizeof(t_arena) + sizeof(t_chunk) * CHUNKS_PER_ARENA;

	/*
	   Allocate memory using mmap. If the requested size isn't that big, we allocate enough memory to hold
	   CHUNKS_PER_ARENA times this size to prepare for future malloc. If the requested size exceeds that value, nearest
	   page size will do.
	*/
	if (size <= SMALL) {
		size = size * 100 + headers_size;
	} else {
		size += sizeof(t_arena) + sizeof(t_chunk);
	}
	size_t rounded_size = size + pagesize - (size % pagesize);
	t_arena *new_arena = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	/* Let's assume that usually the allocation will succeed. But if it doesn't. */
	if (__builtin_expect(new_arena == MAP_FAILED, 0)) {
		errno = ENOMEM;
		pthread_mutex_unlock(mutex);
		return new_arena;
	}

	new_arena->id = self;
	return new_arena;
}

static unsigned long
fast_hash (unsigned long x) {
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;

	return x % HASH_TABLE_SIZE;
}

void *
__malloc (size_t size)
{
	if (size <= 0) return NULL;

	static int				flags = 0;
	static long				pagesize = 0;
	static t_bucket			*aligned_bucket_start = NULL;
	t_arena					*current_arena = NULL;

	/* Get current program thread address and bucket. */
	pthread_t self = pthread_self();

	unsigned long bucket_offset = fast_hash((unsigned long)self) * sizeof(t_bucket);

	/*
	   First malloc may be called simultaneously by different threads. In order to prevent the arena map being created
	   more than once, we place a mutex before reading if arena_map already exists or not. If arena_map doesn't exist,
	   we release the mutex after it's creation to enable other thread access. If it does, the mutex is released
	   instantly. GCC's builtin expect further improves performance as we know there will only be a single time in the
	   program where the comparison holds false.
	*/

	pthread_mutex_lock(&g_arena_mutex);

	if (__builtin_expect(g_arena_map == NULL, 0)) {

		/* Initialize non-runtime constant static variables. This will only be called once. */
		pagesize = sysconf(_SC_PAGESIZE);
		if (getenv("DEBUG") != NULL) flags |= 1 << DEBUG;
		if (getenv("VERBOSE") != NULL) flags |= 1 << VERBOSE;

		/*
		   Each thread will have access to it's own pool of memory to allow for efficient multi-threading.
		   For a thread to access quickly it's arena, we will hash the thread id. Each bucket contains a linked list
		   will several arenas. Arena_map contains an array of HASH_TABLE_SIZE buckets. Lower means less buckets, hence
		   slower access time. Bigger means more buckets, which should reduce collisions and access time, but will take
		   more memory. We use a HASH_TABLE_SIZE value that is a power of 2 and can fill a pagesize as much as possible.
		*/
		size_t map_size = sizeof(t_arena_map) + HASH_TABLE_SIZE * sizeof(t_bucket);
		size_t rounded_size = map_size + pagesize - (map_size % pagesize);

		g_arena_map = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		if (g_arena_map == MAP_FAILED) {
			pthread_mutex_unlock(&g_arena_mutex);
			return NULL;
		}
		g_arena_map->size = rounded_size;

		/* Initialize thread arena. */
		current_arena = create_new_arena(size, pagesize, flags, self, &g_arena_mutex);
		if (current_arena == MAP_FAILED) return NULL;

		/* Align bucket start on 16bytes boundary. */
		aligned_bucket_start = (t_bucket *)(((unsigned long)g_arena_map->bucket_start + 15) & ~0x0f);

		/* Place thread arena in corresponding bucket. */
		((t_bucket *)((unsigned long)aligned_bucket_start + bucket_offset))->arena = current_arena;

		pthread_mutex_unlock(&g_arena_mutex);

		/* Register a call to munmap at program exit. */
		atexit(terminate);

	} else { /* Arena_map exists, so the thread has to look for it's own arena. */

		pthread_mutex_unlock(&g_arena_mutex);

		/*
		   Arena_map allows fast access to the assigned memory area per thread by hashing the thread address.
		   We then need to loop through the bucket's linked list as they may be collisions with the hashing function.
		   If the thread id isn't associated with any of the areas in said linked list, we create a new arena.
		*/

		/* Lock the bucket mutex. */
		t_bucket *bucket = (t_bucket *) ((unsigned long) aligned_bucket_start + bucket_offset);
		pthread_mutex_lock(&bucket->mutex);

		/* If the memory space at the bucket emplacement is empty, we can create one straight away. */
		if (bucket->arena == NULL) {
			current_arena = create_new_arena(size, pagesize, flags, self, &bucket->mutex);
			if (current_arena == MAP_FAILED) return NULL;
			bucket->arena = current_arena;

		} else { /* Otherwise, there is a collision with another thread hash. */

			/*
			   Let's check if there's an arena that belongs to our thread in the bucket.
			   If we don't find a link that matches with the thread, we'll have to create a new link. The anchor will
			   keep in memory the last link to allow us to skip going through the linked list a second time.
			*/

			t_arena *item = bucket->arena, *anchor = NULL;
			while (item) {
				if (pthread_equal(item->id, self)) {
					current_arena = item;
					break;
				}

				if (item->next == NULL) anchor = item;
				item = item->next;
			}

			/* We didn't find an existing arena, we're going to have to create one and add it to the bucket. */
			if (anchor != NULL) {
				current_arena = create_new_arena(size, pagesize, flags, self, &bucket->mutex);
				if (current_arena == MAP_FAILED) return NULL;
				anchor->next = current_arena;
			}
		}

		pthread_mutex_unlock(&bucket->mutex);
	}

	return current_arena;
}
