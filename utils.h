
#ifndef UTILS_H_
#define UTILS_H_

#include "shalloc.h"

/*
 *  Return desired power of 2.
 */
#define pow2(pow)	(1 << (pow))

#define LOG	printf


struct avail_head {
	struct block *prev;
	struct block *next;
};

/*
 *  Find nearest bigger number, which is power of two and return the power.
 */
unsigned long get_pow(unsigned long);

int list_empty(struct avail_head *);

#endif
