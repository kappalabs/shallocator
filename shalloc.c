#define _BSD_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <sys/mman.h>
#include <limits.h>

#include "shalloc.h"
#include "utils.h"


#define MIN(a,b)	(((a)<(b)) ? (a) : (b))
#define MAX(a,b)	(((a)>(b)) ? (a) : (b))

/*
 *  Head of link list with all blocks
 */
// TODO delete
static BLOCK *head = NULL;
static void *mem_begin, *mem_end;

struct mem_pool mp;


static void dump_data(void *ptr, unsigned long size) {
	unsigned long i=0;
	int j, k;
//	if (size > 512) {
//		size = 512;
//	}
	while (i < size) {
		printf("%08x  ", (unsigned int) i);
		for (j=0; j<8 && i<size; j++) {
			for (k=0; k<2 && i<size; k++) {
				printf("%02x", ((char *)(ptr))[i++]&0xff);
			}
			printf(" ");
		}
		printf("\n");
	}
}

static void print_block_info(struct block *ptr) {
	struct block *b = ptr;
	printf("pos = %p, state = %u, k_size = %lu\n", ptr, b->state, b->k_size);
	printf("data =\n");
	dump_data(B_DATA(ptr), B_DATA_SIZE(ptr));
	printf("\n");
}

static int init_pool() {
	/* Initialize current heap break address */
	if ((mp.base_addr = sbrk(0)) == (void *) -1) {
		perror("sbrk");
		return -1;
	}

	/* Prepare space for first block containing whole starting memory pool */
	/* We need space for the block head and at least one byte worth of data */
	unsigned long min_size = get_pow(B_SIZE + 1); 
	/* First block will contain mp.avail array */
	unsigned long init_size = min_size; 
//	/* Find proper size of the first block for the avail heads array, which has size pool_size + 1 TODO + 1 for the next item? */
//	while (pow2(init_size) < (1+1+init_size)*(sizeof(struct avail_head *) + sizeof(struct avail_head)) + B_SIZE) {
//		init_size++;
//	}

	/* Logarithm of the space, we can adress */
	mp.max_size = CHAR_BIT*sizeof(void *);
	/* We want to have available pointers to all the memory we can adress */
	init_size = get_pow(B_SIZE + (mp.max_size + 1)*(sizeof(struct avail_head)));
	if (sbrk(pow2(init_size)) == (void *) -1) {
		perror("sbrk");
		return -1;
	}
	mp.pool_size = init_size;

	/* Initialize the first block, which will contain avail array */
	struct block *first_b;
	first_b = (struct block *) mp.base_addr;
	first_b->state = USED;
	first_b->k_size = init_size;

	LOG("--## POOL INFO ##--\n");
	LOG("Memory begin at base_addr = %p\n", mp.base_addr);
	LOG("Size of the first block is init_size=2^%lu=%uB, diff=%lu\n", init_size, pow2(init_size), sbrk(0)-mp.base_addr);
	LOG("Current break address is %p\n", sbrk(0));

	/* Init cyclic linked list */
	mp.avail = (struct avail_head *) B_DATA(first_b);
	LOG("mp.avail addr = %p\n", mp.avail);
	int i;
	for (i=0; i <= mp.max_size; i++) {
		mp.avail[i].next = (struct block *) &mp.avail[i];
		mp.avail[i].prev = (struct block *) &mp.avail[i];
	}

//	for (i=0; i<mp.max_size; i++) {
//		LOG("%d. empty = %d\n", i, list_empty(&mp.avail[i]));
//	}

	//first_b->prev = (struct block *) mp.avail[first_b->k_size]; //first_b;
	//first_b->next = (struct block *) mp.avail[first_b->k_size]; //first_b;
	first_b->prev = first_b;
	first_b->next = first_b;

	//print_block_info(first_b);
	LOG("--## END OF POOL INFO ##--\n");

	return 0;
}

static void *get_cur_brk() {
	void *cur;
	if ((cur = sbrk(0)) == (void *) -1) {
		perror("sbrk");
		return NULL;
	}
	return cur;
}

static void init_block(BLOCK *b, void *ptr, size_t size) {
	b->state = FREE;
	/* TODO mozno logaritmit dvojkove */
	//b->b_size = 42; /* neni potreba duplikovat..., B_SIZE + size; */
	b->k_size = get_pow(B_SIZE + size);
	//b->d_size = size;
	//b->data = (void *) (ptr + B_SIZE);
	b->next = NULL;
	b->prev = NULL;
}

void *shalloc(size_t size) {
	LOG("This function(shalloc()) is WIP!\n");

	/* Pool not initialized */
	if (mp.base_addr == NULL) {
		/* Try to initialize it */
		if (init_pool() == -1) {
			errno = ENOMEM;
			return NULL;
		}
	}

	/* Nothing to do */
	if (size == 0) {
		/* Is this standard behavior? */
		errno = 0;
		return NULL;
	}

	/* Size in power of 2, which needs to be reserved */
	unsigned long size_pow = get_pow(B_SIZE + size);
	LOG("Request for 2^%lu = %uB\n", size_pow, pow2(size_pow));
	/* Size of the first suitable block which is available (in pow of 2) */
	unsigned long j;
	/* Find the value of j */
	LOG("--## FINDING J ##--\n");
	for (j=size_pow; j < mp.pool_size; j++) {
		if (!list_empty(&mp.avail[j])) {
			break;
		}
		LOG("j=%lu not suitable\n", j);
	}
	/* Do we need to enlarge the pool? - by making buddy for existing largest block */
	LOG("--## ENLARGING ##--\n");
	while (list_empty(&mp.avail[j])) {
		LOG("j=%lu <= pool_size=%lu <= max=%lu\n", j, mp.pool_size, mp.max_size);
		/* Cannot adress this amount of memory */
		if (mp.pool_size >= mp.max_size) {
			LOG("Maximum size reached!\n");
			errno = ENOMEM;
			return NULL;
		}
		LOG("Enlarging the pool.\n");
		void *new_addr;
		if ((new_addr = sbrk(pow2(mp.pool_size))) == (void *)-1) {
			LOG("sbrk error!\n");
			errno = ENOMEM;
			return NULL;
		}
		/* Pool was enlarged, we have twice as much space */
		mp.pool_size++;
		/* New memory block, buddy, will live in this space */
		struct block *nb = (struct block *) new_addr;
		nb->state = FREE;
		nb->k_size = mp.pool_size-1;
	
		/* Avail array must be edited, we've got new free block of size 2^(nb->k_size) */
		struct block *p;
		p = mp.avail[nb->k_size].next;
		nb->next = p;
		p->prev = nb;
		nb->prev = (struct block *) &mp.avail[nb->k_size];
		mp.avail[nb->k_size].next = nb;

		/* Is it enough? */
		if (j < mp.pool_size && list_empty(&mp.avail[j])) {
			//j++;
		}
	}
	/* We've got j value set */
	LOG("Nice, got j=%lu, ready to be divided?\n", j);
	int i;
	for (i=0; i<16; i++) {
		LOG("%d. empty = %d\n", i, list_empty(&mp.avail[i]));
		if (!list_empty(&mp.avail[i])) {
			//print_block_info(mp.base_addr);
		}
	}

	LOG("--## REMOVING BLOCK FROM AVAIL ARRAY ##--\n");
	/* Remove this block from avail array */
	struct block *l, *p;
	l = mp.avail[j].prev;
	p = l->prev;
	mp.avail[j].prev = p;
	LOG("point %p, point %p\n", &mp.avail[j], p);
	p->next = (struct block *) &mp.avail[j];
	l->state = USED;

	/* Now we need to divide the block if space in it is too large */
	LOG("--## DIVIDING BLOCK ##--\n");
	while (j != size_pow) {
		LOG("divination for j=%lu\n", j);
		j--;
		p = (struct block *)((unsigned long)l + pow2(j));
		LOG("pointering to %p\n", p);
		p->state = FREE;
		p->k_size = j;
		p->prev= (struct block *) &mp.avail[j];
		p->next = (struct block *) &mp.avail[j];
		/* Add this block into avail array */
		mp.avail[j].prev = p;
		mp.avail[j].next = p;
	}
	for (i=0; i<16; i++) {
		LOG("%d. empty = %d\n", i, list_empty(&mp.avail[i]));
		if (!list_empty(&mp.avail[i])) {
			//print_block_info(mp.base_addr);
		}
	}
	return B_DATA(l);
}

void *shalloc_deprecated(size_t size) {
	printf("This function(shalloc_deprecated()) is deprecated!\n");
	/* Nothing to do */
	if (size == 0) {
		return NULL;
	}
	/* Initialize heap buffer edges */
	if (mem_begin == NULL) {
		mem_begin = get_cur_brk();
		mem_end = mem_begin;
	}
	/* Allocation unsuccesfull */
	if (mem_begin == NULL) {
		return NULL;
	}
	/* Size of the whole buddy */
	int w_size = pow2(get_pow(B_SIZE + size));
	/* Is allocation necessary? */
	if (mem_begin + w_size > mem_end) {
		printf("shalloc() moving break\n");
		/* Move the break then */
		if (brk(mem_begin + w_size) == -1) {
			perror("brk");
			return NULL;
		}
	}
	/* Initialize new BLOCK */
	BLOCK *b = (BLOCK *) mem_begin;
	init_block(b, mem_begin, size);
	b->state = USED;

	/* Modify the linked list */
	if (head == NULL) {
		head = b;
	} else {
		BLOCK *last = head;
		while (last->next != NULL) {
			last = (BLOCK *)last->next;
		}
		b->prev = last;
		last->next = b;
	}

	/* Adjust heap buffer edges */
	mem_begin += w_size;
	mem_end = get_cur_brk();

	// TODO: pozor mem_begin je upravovano chvili pred!!!
	return (void *)(B_SIZE + mem_begin - w_size);
}

void *shcalloc(size_t nmemb, size_t size) {
	printf("Unimplemented function!\n");
	return NULL;
}

void *reshalloc(void *ptr, size_t size) {
	printf("Unimplemented function!\n");
	return NULL;
}

void *reshcalloc(void *ptr, size_t size) {
	printf("Unimplemented function!\n");
	return NULL;
}

void shee(void *ptr) {
	printf("This function (shee()) is WIP!\n");
	struct block *b = B_HEAD(ptr);
	b->state = FREE;
	printf("Freed memory info:\n");
	print_block_info(b);
}

typedef char type;

int main(int argc, char **argv) {
	printf("Block size = %lu\n", B_SIZE);

	type *p1, *p2, *p3;
	if ((p1 = (type *) shalloc(10 * sizeof(type))) == NULL) {
		err(2, "shalloc");
	}
	if ((p2 = (type *) shalloc(5 * sizeof(type))) == NULL) {
		err(2, "shalloc");
	}
	if ((p3 = (type *) shalloc(32 * sizeof(type))) == NULL) {
		err(2, "shalloc");
	}
	int i;
	for (i=0; i<10; i++) {
		p1[i] = i;
	}
	print_block_info(B_HEAD(p1));
	for (i=0; i<5; i++) {
		p2[i] = i;
	}
	for (i=0; i<32; i++) {
		p3[i] = i;
	}

	shee(p3);
	if ((p3 = (type *) shalloc(32 * sizeof(type))) == NULL) {
		err(2, "shalloc");
	}
	for (i=0; i<32; i++) {
		p3[i] = i;
	}
	shee(p3);

	shee(p2);
	shee(p1);
	
	return (0);
}
