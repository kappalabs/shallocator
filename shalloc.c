#define _BSD_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

/* In kB */
#define MIN_BLOCK_SIZE	64
#define MIN(a,b)	(((a)<(b)) ? (a) : (b))
#define MAX(a,b)	(((a)>(b)) ? (a) : (b))

/*
 *  Useful states of memory block
 */
typedef enum m_state { 
	FREE, USED, FULL, ZERO
} M_STATE;

/*
 *  Description of one memory block on heap
 *  This structure is head of each such block
 */
typedef struct block {
	M_STATE state;
	char    b_size; /* TODO 2^b_size kB = real size of this whole block, (will always be power of 2 in BUDDY) */
	size_t  d_size; /* desired size of data segment */
	void   *data;
	struct block *prev;
	struct block *next;
} BLOCK;

#define B_SIZE	(unsigned int)sizeof(struct block)

/*
 *  Head of link list with all blocks
 */
static BLOCK *head = NULL;

/*
 *  Adresses of buffer (preallocated unused memory)
 *  before the break on heap.
 */
static void *mem_begin = NULL;
static void *mem_end = NULL;


static void dump_data(void *ptr, size_t size) {
	size_t i, j, k;
	i=0;
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

static void print_block_info(void *ptr) {
	BLOCK *b = (BLOCK *) ptr;
	printf("pos = %p, state = %u, b_size = %u, d_size = %u\n", ptr, b->state, b->b_size, (unsigned int) b->d_size);
	printf("data =\n");
	dump_data(b->data, b->d_size);
	printf("\n");
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
	b->b_size = 42; /* neni potreba duplikovat..., B_SIZE + size; */
	b->d_size = size;
	b->data = (void *) (ptr + B_SIZE);
	b->next = NULL;
	b->prev = NULL;
}

void *shalloc(size_t size) {
	printf("This function(shalloc()) is WIP!\n");
	/* Nothing to do */
	if (size == 0) {
		return NULL;
	}
	/* Initialize heap buffer edges */
	if (mem_begin == NULL) {
		mem_begin = get_cur_brk();
		mem_end = mem_begin;
	}
	/* Size of the whole chunk to be saved */
	int w_size = B_SIZE + size;
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
			last = last->next;
		}
		b->prev = last;
		last->next = b;
	}

	/* Adjust heap buffer edges */
	mem_begin += w_size;
	mem_end = get_cur_brk();

	return b->data;
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
	BLOCK *b = (BLOCK *) (ptr - B_SIZE);
	b->state = FREE;
	printf("Freed memory info:\n");
	print_block_info(b);
}

typedef short type;

int main(int argc, char **argv) {
	printf("Block size = %u\n", B_SIZE);

	type *p1 = (type *) shalloc(10 * sizeof(type));
	type *p2 = (type *) shalloc(5 * sizeof(type));
	type *p3 = (type *) shalloc(32 * sizeof(type));
	int i;
	for (i=0; i<10; i++) {
		p1[i] = i;
	}
	for (i=0; i<5; i++) {
		p2[i] = i;
	}
	for (i=0; i<32; i++) {
		p3[i] = i;
	}

	shee(p3);
	p3 = (type *) shalloc(32 * sizeof(type));
	for (i=0; i<32; i++) {
		p3[i] = i;
	}
	shee(p3);

	shee(p2);
	shee(p1);
	
	return (0);
}
