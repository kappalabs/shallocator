
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../shalloc.h"
#include "../net_utils.h"


int main(int argcchar, char **argv) {
	char *text;
	if ((text = (char *)shcalloc(1, 256)) == NULL) {
		perror("shcalloc");
		return 1;
	}
	printf("text = >>%s<<\n", text);
	strcpy(text, "data ke zpracovani");
	printf("text = >>%s<<\n", text);
	if ((text = shwapoff(text)) == NULL) {
		printf("Can't swapoff!\n");
		return 1;
	}
	printf("text is now unavailable\n");
	if ((text = shwapon(text)) == NULL) {
		printf("Can't swapon!\n");
		return 1;
	}
	printf("text = >>%s<<\n", text);

	return 0;
}
