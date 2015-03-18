#ifndef SHALLOC_H_
#define SHALLOC_H_

#include <stddef.h>

/*
 *   heap break ----> --------  ^   
 *                    |      |  |
 *                    . HEAP .  | 2^pool_size
 *                    |      |  |
 *   mp.base_addr --> --------  v
 *
 *   Structure mem_pool contains all basic needed information about allocated memory under heap break.
 */
struct mem_pool {
	/* Adress of the beginning of heap. */
	void *base_addr;
	/* Size of allocated memory (in power of 2) */
	unsigned long pool_size;
	/* Maximum addressable space */
	unsigned long max_size;
	/* Size of the first block, which contains 'avail' array (in pow of 2) */
	unsigned long init_size;

	/*
	 *  Array of heads for cyclic linked lists,
	 *  each list contains only free blocks
	 *  of size 2^i for list starting at avail[i].
	 */
	struct avail_head *avail;
};

/*
 *  Useful states of memory block
 */
typedef enum m_state { 
	FREE, USED, ZERO, UNUSED
} M_STATE;

/*
 *  Description of one memory block on heap.
 */
typedef struct block {
	struct block *prev;
	struct block *next;

	M_STATE state;
	unsigned long k_size; /* Size of this whole block, (will always be power of 2) */
} BLOCK;


/* Size of every memory block header */
#define B_SIZE	(unsigned long) sizeof(struct block)
/* Pointer to the data section for given block */
#define B_DATA(p)	(void *) ((unsigned long)(p) + B_SIZE)
/* Length of data in block from given pointer */
#define B_DATA_SIZE(p)	(unsigned long) (pow2((p)->k_size) - B_SIZE)
/* From pointer to data section it returns pointer to the block structure */
#define B_HEAD(p)	(struct block *) ((unsigned long)(p) - B_SIZE)


/**
 *  Allocate 'size' bytes and return pointer to the allocated memory.
 *  The memory is not initialized. If 'size' is 0, shalloc() returns NULL.
 *  NULL is also returned on error.
 */
extern void *shalloc(size_t size);

/**
 *  Allocate space for 'nmemb' objects of 'size' bytes and return pointer to the
 *  allocated memory. The memory is initialized to zero. If either 'nmemb' or
 *  'size' is zero, shcalloc() returns NULL.
 *  NULL is also returned on error.
 */
extern void *shcalloc(size_t nmemb, size_t size);

/**
 *  Change the size of the memory block pointed to by 'ptr' to 'size' bytes.
 *  The previous content will be unchanged, added memory will not be initialized.
 *  If 'ptr' is NULL, then the call is equivalent to shalloc(size).
 *  If 'size' is zero, then the call is equivalent to free(ptr).
 *  Pointer 'ptr' must have been returned by calling another shallocator
 *  allocation function or it is NULL.
 *  Returns pointer to the memory block of required size with data unchanged,
 *  on error, or if size was zero, NULL is returned.
 */
extern void *reshalloc(void *ptr, size_t size);

/**
 *  Mostly the same as reshalloc(). The previous content will be unchanged,
 *  in the contrast with reshalloc(), added memory will be initialized with zeros.
 */
extern void *reshcalloc(void *ptr, size_t size);

/**
 *  Function frees the memory at position 'ptr', which is either NULL or pointer
 *  which was obtained from some shallocator allocation function.
 *  If 'ptr' is NULL, no operation is performed.
 *  If shee() is called improperly, its behavior is undefined.
 */
extern void shee(void *ptr);

/**
 *  Malloc-like functions wrappers, some libraries used here are calling them, which makes fight
 *  between two different heap allocators. This way only Shallocator is used.
 */
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *calloc(size_t nmemb, size_t size);
extern void *realloc(void *ptr, size_t size);

#endif
