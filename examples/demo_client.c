
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../shalloc.h"
#include "../net_utils.h"


int main(int argcchar, char **argv) {
	char *data1, *data2; /* Data will be stored here */
	void *swp1, *swp2; /* Used to retrieve shwapped data back */

	/* Allocate space for data with library function shalloc() or it's variation */
	if ((data1 = (char *)shcalloc(1, 128)) == NULL) {
		perror("shcalloc");
		return 1;
	}
	if ((data2 = (char *)shcalloc(1, 64)) == NULL) {
		perror("shcalloc");
		return 1;
	}

	/* Make preprocessing, or just work with the data as usual */
	printf("'data1' after shallocation: >>%s<<\n", data1);
	strcpy(data1, "This string will be shwapped out");
	printf("'data1' before shwapoff(): >>%s<<\n", data1);

	/* Send data to remote server */
	if ((swp1 = shwapoff(data1)) == NULL) {
		printf("Can't shwapoff data1!\n");
		shee(data1);
		return 0;
	}
	if ((swp2 = shwapoff(data2)) == NULL) {
		printf("Can't shwapoff data2!\n");
		shee(data2);
		return 0;
	}

	/* In this part, you are not able to use the 'data1' and 'data2' pointers */
	printf("'data1' and 'data2' is now unavailable, stored on the server side\n");

	/* Retrieve the data back from server, using previously given pointer */
	/* It does not matter in which order you do that */
	if ((data2 = shwapon(swp2)) == NULL) {
		printf("Can't shwapon 'data2'!\n");
		return 0;
	}
	if ((data1 = shwapon(swp1)) == NULL) {
		printf("Can't shwapon 'data1'!\n");
		return 0;
	}


	/*
	 *  New pointer is going to be returned, with the previous, or processed data
	 *   - that is dependent on 'processor()' function on the server, which was used
	 */
	printf("'data1' after shwapon(): >>%s<<\n", data1);
	printf("'data2' after shwapon(): >>%s<<\n", data2);
	shee(data1);
	shee(data2);

	return 0;
}
