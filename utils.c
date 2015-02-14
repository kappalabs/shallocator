#include <stdio.h>

#include "utils.h"

unsigned long get_pow(unsigned long value) {
	unsigned long pow=0;
	unsigned long tmp=1;
	while (tmp < value) {
		tmp *= 2;
		pow++;
	}

	return pow;
}

int list_empty(struct avail_head *head) {
	return  head->next == (struct block *) head;
}
