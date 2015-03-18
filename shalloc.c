#define _BSD_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <sys/mman.h>
#include <limits.h>
#include <pthread.h>

#include "shalloc.h"
#include "utils.h"
#include "net_utils.h"


static struct mem_pool mp;
/*
 *  Thread that will zero out available memory.
 *  It will also take care of destroying the memory pool structure.
 */
static pthread_t cleaner_th;
/*
 *  Locks for safe work with memory pool structure with multiple threads.
 */
//pthread_mutex_t mp_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t mp_condvar = PTHREAD_COND_INITIALIZER;

void thread_cleanup(void *arg) {
	LOG("--## THREAD CLEANUP ##--\n");
	fflush(stdout);
//	pthread_mutex_destroy(&mp_mutex);
//	pthread_cond_destroy(&mp_condvar);
	pthread_exit(NULL);
}

void *cleaning_work(void *ptr) {
	LOG("--## CLEANER THREAD INIT ##--\n");
	pthread_cleanup_push(thread_cleanup, "Cleaning thread's handler");
	while (1) {
	}
	pthread_cleanup_pop(1);
	pthread_exit(NULL);
	//return NULL;
}

void print_avail_map() {
	if (mp.base_addr == NULL) {
		return;
	}
	
	int i;
	for (i=0; i <= mp.pool_size; i++) {
		LOG("%02d  ", i);
	} LOG("\n");
	for (i=0; i <= mp.pool_size; i++) {
		LOG("%02d  ", list_len(&mp.avail[i]));
	} LOG("\n");
}

static int init_pool() {
	/* Get current heap break address */
	if ((mp.base_addr = sbrk(0)) == (void *) -1) {
		perror("sbrk");
		return -1;
	}

	/* First block on heap will contain 'avail' array */
	/* First-block initialization for the avail array containing heads of lists with available memory, it has length of (max pool size + 1) */
	/* Logarithm of the space, we can adress */
	mp.max_size = CHAR_BIT*sizeof(void *);
	/* We want to have available pointers to all the memory we can adress */
	mp.init_size = get_pow(B_SIZE + (mp.max_size + 1)*(sizeof(struct avail_head)));
	if (sbrk(pow2(mp.init_size)) == (void *) -1) {
		perror("sbrk");
		return -1;
	}
	mp.pool_size = mp.init_size;

	/* Initialize the first block, which will contain 'avail' array */
	struct block *first_b;
	first_b = (struct block *) mp.base_addr;
	first_b->state = USED;
	first_b->k_size = mp.init_size;

	LOG("--## POOL INIT INFO ##--\n");
	LOG("## Heap memory begins at base_addr = %p\n", mp.base_addr);
	LOG("## Size of the first block is init_size = 2^%lu = %luB\n", mp.init_size, pow2(mp.init_size));
	LOG("## Maximum pool size is max_size = 2^%lu = %luB\n", mp.max_size, -1L);

	/* Init 'avail', array of cyclic linked list */
	mp.avail = (struct avail_head *) B_DATA(first_b);
	int i;
	for (i=0; i <= mp.max_size; i++) {
		mp.avail[i].next = (struct block *) &mp.avail[i];
		mp.avail[i].prev = (struct block *) &mp.avail[i];
	}

	first_b->prev = (struct block *) &mp.avail[first_b->k_size];
	first_b->next = (struct block *) &mp.avail[first_b->k_size];

	//TODO WIP
	int err;
	if ((err = pthread_create(&cleaner_th, NULL, cleaning_work, NULL)) != 0) {
		printf("pthread_create() ERROR %d!\n", err);
		return -1;
	}
	if ((err = pthread_detach(cleaner_th)) != 0) {
		printf("pthread_detach() ERROR %d!\n", err);
		return -1;
	}
	//TODO WIP
	LOG("--## END OF POOL INIT INFO ##--\n");

	return 0;
}

static void *alloc(unsigned long size_pow, enum m_state state) {
	/* Pool not initialized */
	if (mp.base_addr == NULL) {
		/* Try to initialize it */
		if (init_pool() == -1) {
			errno = ENOMEM;
			return NULL;
		}
	}

//	LOG("BEFORE ALLOC\n");
//	print_avail_map();

	/* Size in power of 2, which needs to be reserved */
	//LOG("Request for 2^%lu = %luB\n", size_pow, pow2(size_pow));
	/* Size of the first suitable block which is available (in pow of 2) */
	unsigned long j;
	/* Find the value of 'j' in available memory space if possible */
	//LOG("--## FINDING J ##--\n");
	for (j=size_pow; j < mp.pool_size; j++) {
		if (!list_empty(&mp.avail[j])) {
			break;
		}
		//LOG("j=%lu not suitable\n", j);
	}
	/* Do we need to enlarge the pool? - by making buddy for existing largest block */
	//LOG("--## ENLARGING ##--\n");
	while (list_empty(&mp.avail[j])) {
		//LOG("j=%lu <= pool_size=%lu <= max=%lu\n", j, mp.pool_size, mp.max_size);
		/* Cannot adress this amount of memory */
		if (get_pow((unsigned long)mp.base_addr) + size_pow >= mp.max_size) {
		//TODO ktery pouzit?
		//if (mp.max_size >= mp.max_size) {
			LOG("Maximum size reached!\n");
			errno = ENOMEM;
			return NULL;
		}
		//LOG("Enlarging the pool.\n");
		void *new_addr;
		if ((new_addr = sbrk(pow2(mp.pool_size))) == (void *)-1) {
			LOG("sbrk pool-enlarge error!\n");
			errno = ENOMEM;
			return NULL;
		}
		//TODO bez prepsani zpusobuje neporadek s valgrindem
		//memset(new_addr, 0, pow2(mp.pool_size));

		/* Pool was enlarged, we have twice as much space */
		mp.pool_size++;
		/* New memory block, buddy for previous block, will live in this space */
		//LOG("new_addr = %p\n", new_addr);
		struct block *nb = (struct block *) new_addr;
		nb->state = FREE;
		nb->k_size = mp.pool_size - 1;
		//LOG("k_size = %lu\n", nb->k_size);
	
		/* Avail array must be edited, we've got new free block of size 2^(nb->k_size) */
		struct block *p;
		p = mp.avail[nb->k_size].next;
		//LOG("p point %p head to %p\n", p, &mp.avail[nb->k_size]);
		nb->next = p;
		p->prev = nb;
		nb->prev = (struct block *) &mp.avail[nb->k_size];
		mp.avail[nb->k_size].next = nb;
	}

	/* We now have the 'j' value set */
	//LOG("--## REMOVING BLOCK FROM AVAIL ARRAY ##--\n");
	/* Remove this block from avail array */
	struct block *l, *p;
	l = mp.avail[j].prev;
	//LOG("L position = %p\n", l);
	p = l->prev;
	mp.avail[j].prev = p;
	p->next = (struct block *) &mp.avail[j];
	enum m_state block_state = l->state;
	l->state = USED;

	/* Now we need to divide the block if space in it is too large */
	//LOG("--## DIVIDING BLOCK ##--\n");
	while (j != size_pow) {
		//LOG("divination for j=%lu\n", j);
		j--;
		p = (struct block *)((unsigned long)l + pow2(j));
		//LOG("pointering to %p\n", p);
		p->state = FREE;
		p->k_size = j;
		p->prev = (struct block *) &mp.avail[j];
		p->next = (struct block *) &mp.avail[j];
		/* Add this block into avail array */
		mp.avail[j].prev = p;
		mp.avail[j].next = p;
	}
	l->k_size = size_pow;
	
	/* Does the memory need to be cleared? */
	if (state == ZERO && block_state != ZERO) {
		memset(B_DATA(l), 0, B_DATA_SIZE(l));
	}

//	LOG("AFTER ALLOC\n");
//	print_avail_map();
//	LOG("\n");

	return B_DATA(l);
}

void *shalloc(size_t size) {
	/* Nothing to do */
	if (size == 0) {
		return NULL;
	}

	return alloc(get_pow(B_SIZE + size), FREE);
}

void *shcalloc(size_t nmemb, size_t size) {
	/* Nothing to do */
	if (nmemb == 0 || size == 0) {
		return NULL;
	}

	return alloc(get_pow(B_SIZE + nmemb*size), ZERO);
}

static void *ralloc(void *ptr, size_t size, enum m_state state) {
	//LOG("Request for size change to %zd\n", size);
	
	if (size == 0) {
		shee(ptr);
		return NULL;
	}
	if (ptr == NULL) {
		return shalloc(size);
	}

	struct block *b = B_HEAD(ptr);
	unsigned long size_pow = get_pow(B_SIZE + size);
	/* No need for allocation, size would be the same */
	if (b->k_size == size_pow) {
		return ptr;
	}

	void *new_ptr;
	if (state == ZERO) {
		new_ptr = shcalloc(1, size);
	} else {
		new_ptr = shalloc(size);
	}
	/* In case of failure, previous data is lost */
	if (new_ptr == NULL) {
		shee(ptr);
		return NULL;
	}
	/* Previous data must be preserved, if possible */
	memcpy(new_ptr, ptr, MIN(B_DATA_SIZE(b), B_DATA_SIZE(B_HEAD(new_ptr))));
	shee(ptr);

	return new_ptr;
}

void *reshalloc(void *ptr, size_t size) {
	return ralloc(ptr, size, FREE);
}

void *reshcalloc(void *ptr, size_t size) {
	return ralloc(ptr, size, ZERO);
}

static struct block *get_buddy(struct block *b, unsigned long pow) {
	/* Relative addresses to the begining of heap memory */
	unsigned long base_block;
	unsigned long base_buddy;
	
	base_block = (unsigned long) b - (unsigned long) mp.base_addr;
	base_buddy = base_block ^ pow2(pow);

	return (struct block *) (base_buddy + mp.base_addr);
}

void shee(void *ptr) {
	/* Nothing to do */
	if (ptr == NULL) {
		return;
	}

//	LOG("BEFORE SHEE AVAIL MAP\n");
//	LOG("Mem_pool size = 2^%luB\n", mp.pool_size);
//	print_avail_map();

	struct block *b = B_HEAD(ptr);
	struct block *buddy = NULL;
	b->state = FREE;

//	LOG("Going to free:\n");
	//LOG("Shee block of size 2^%luB = %luB\n", b->k_size, pow2(b->k_size));
//	print_block_info(b);

	/* Combine free block if buddy-possible */
	unsigned long k_size = b->k_size;
	while (k_size < mp.pool_size) {
		/* Find my buddy */
		buddy = get_buddy(b, k_size);

		/* Is buddy available? */
		if (buddy->state == USED) {
			break;
		}
		/* Is buddy used somewhere inside? */
 		if (buddy->k_size != k_size) {
			break;
		}
	
		/* Combine buddies together */
		buddy->prev->next = buddy->next;
		buddy->next->prev = buddy->prev;
		k_size++;
		if (buddy < b) {
			b = buddy;
		}
	}

	/* Put block on proper 'avail' list */
	b->state = FREE;
	b->k_size = k_size;
	struct block *p;
	p = mp.avail[b->k_size].next;
	b->next = p;
	p->prev = b;
	b->prev = (struct block *) &mp.avail[b->k_size];
	mp.avail[b->k_size].next = b;


//	LOG("AFTER SHEE AVAIL MAP\n");
//	print_avail_map();
//	LOG("\n");

	//TODO: Bude provadet jine vlakno - vyresi se problem uklizeni poolu
	//  po neuspesnem shallocu, ktery byl proveden jako prvni
	/* Shrink down memory pool if possible */
	int enableDestroy = 1;
	while (enableDestroy && !list_empty(&mp.avail[mp.pool_size-1])) {
		b = mp.avail[mp.pool_size-1].next;
		buddy = get_buddy(b, mp.pool_size-1);
		/* Is 'b' free block on the top of the heap with half of the pool size? */
		if (b < buddy) {
			break;
		}

		/* Correct the 'avail' array of old pool */
		mp.avail[mp.pool_size].prev = (struct block *) &mp.avail[mp.pool_size];
		mp.avail[mp.pool_size].next = (struct block *) &mp.avail[mp.pool_size];

		LOG("--## SHRINKING ##--\n");
		/* Then we can shrink the heap/pool */
		if (sbrk(-pow2(b->k_size)) == (void *) -1) {
			perror("sbrk");
		}
		mp.pool_size--;
		LOG("Pool size is now %lu\n", mp.pool_size);

		/* Correct the 'avail' array */
		mp.avail[mp.pool_size].prev = (struct block *) &mp.avail[mp.pool_size];
		mp.avail[mp.pool_size].next = (struct block *) &mp.avail[mp.pool_size];

		/* If only the first block with 'avail' array is on heap, we can clean-up */
		if (mp.pool_size == mp.init_size) {
			LOG("--## POOL DESTROYED ##--\n");
			if (brk(mp.base_addr) == -1) {
				perror("brk");
			}
			mp.base_addr = NULL;

			break;
		}
	}
}

void *malloc(size_t size) {
	LOG("someone called malloc()\n");
	return shalloc(size);
}
void free(void *ptr) {
	LOG("someone called free()\n");
	shee(ptr);
}
void *calloc(size_t nmemb, size_t size) {
	LOG("someone called calloc()\n");
	return shcalloc(nmemb, size);
}
void *realloc(void *ptr, size_t size) {
	LOG("someone called realloc()\n");
	return reshalloc(ptr, size);
}
