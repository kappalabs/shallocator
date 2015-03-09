#include <stdio.h>

#include "../shalloc.h"


typedef short type;

int main(int argc, char **argv) {
	printf("Block size = %lu\n", B_SIZE);

//	struct clients *cfg;
//	printf("Printing config file input\n");
//	cfg = read_config("demo.cfg"); 
//	dump_config(cfg);
//	free_config(cfg);
//
//	return 0;

	int i;
	type *p1, *p2, *p3, *p4;
	int bools[] = {0, 0, 0, 0};

	if ((p1 = (type *) shcalloc(1024*1024*128, sizeof(type))) == NULL) {
		perror("shcalloc");
		bools[0] = 0;
	} else {
		bools[0] = 1;
		for (i=0; i<1024*1024*128; i++) {
			p1[i] = i;
		}
		//print_block_info(B_HEAD(p1));
	}
	bools[0] = 0;
	shee(p1);
	return 0;

	if ((p1 = (type *) shalloc(5 * sizeof(type))) == NULL) {
		perror("shalloc");
		bools[0] = 0;
	} else {
		bools[0] = 1;
		for (i=0; i<5; i++) {
			p1[i] = i;
		}
	}

	if ((p2 = (type *) shalloc(5 * sizeof(type))) == NULL) {
		perror("shalloc");
		bools[1] = 0;
	} else {
		bools[1] = 1;
		for (i=0; i<5; i++) {
			p2[i] = i;
		}
	}

	if ((p4 = (type *) shalloc(5 * sizeof(type))) == NULL) {
		perror("shalloc");
		bools[3] = 0;
	} else {
		bools[3] = 1;
		for (i=0; i<5; i++) {
			p4[i] = i;
		}
	}

	bools[1] = 0;
	shee(p2);
	bools[0] = 0;
	shee(p1);
	bools[3] = 0;
	shee(p4);

	if ((p3 = (type *) shalloc(32 * sizeof(type))) == NULL) {
		perror("shalloc");
	} else {
		for (i=0; i<32; i++) {
			p3[i] = i;
		}
		shee(p3);
	}

	if ((p3 = (type *) shalloc(32 * sizeof(type))) == NULL) {
		perror("shalloc");
	} else {
		for (i=0; i<32; i++) {
			p3[i] = i;
		}
		shee(p3);
	}

	if (bools[1]) {
		shee(p2);
	}
	if (bools[0]) {
		shee(p1);
	}
	
	if ((p3 = (type *) shalloc(32 * sizeof(type))) == NULL) {
		perror("shalloc");
		bools[2] = 0;
	} else {
		bools[2] = 1;
		for (i=0; i<32; i++) {
			p3[i] = i;
		}
	}
	if ((p3 = (type *) reshalloc(p3, 128 * sizeof(type))) == NULL) {
		perror("reshalloc");
		bools[2] = 0;
	}
	if ((p3 = (type *) reshcalloc(p3, 32 * sizeof(type))) == NULL) {
		perror("reshcalloc");
		bools[2] = 0;
	}
	if (bools[2]) {
		shee(p3);
	}

	return (0);
}
