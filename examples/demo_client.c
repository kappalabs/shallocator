
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../shalloc.h"
#include "../net_utils.h"


int main(int argcchar, char **argv) {
	char *text; /* Data will be stored here */
	void *swp; /* Used to retrieve shwapped data back */

	/* Allocate space for data with library function shalloc() or it's variation */
	if ((text = (char *)shcalloc(1, 256)) == NULL) {
		perror("shcalloc");
		return 1;
	}

	/* Make preprocessing of data */
	printf("text = >>%s<<\n", text);
	strcpy(text, "This string will be shwapped out");
	printf("text = >>%s<<\n", text);

	/* Send data to remote server */
	if ((swp = shwapoff(text)) == NULL) {
		printf("Can't shwapoff!\n");
		shee(text);
		return 1;
	}

	/* In this part, you are not able to use the 'text' pointer */
	printf("text is now unavailable, stored on server side\n");

	/* Retrieve the data back from server, using previously given pointer */
	if ((text = shwapon(swp)) == NULL) {
		printf("Can't shwapon!\n");
		return 1;
	}

	/*
	 *  New pointer is going to be returned, with the previous, or processed data
	 *   - that is dependent on 'processor()' function on the server, which was used
	 */
	printf("text = >>%s<<\n", text);
	shee(text);

	return 0;
}
