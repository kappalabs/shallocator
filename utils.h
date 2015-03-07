
#ifndef UTILS_H_
#define UTILS_H_

#include "shalloc.h"

/*
 *  Return desired power of 2.
 */
#define pow2(pow)	((unsigned long)1 << (pow))

#define LOG	printf


struct avail_head {
	struct block *prev;
	struct block *next;
};

/*
 *  Find nearest bigger number, which is power of two and return the power.
 */
extern unsigned long get_pow(unsigned long);

extern int list_empty(struct avail_head *);

extern int list_len(struct avail_head *);

extern void dump_data(void *ptr, unsigned long size);

extern void print_block_info(struct block *ptr);

extern char *read_line(int fd);

#endif
