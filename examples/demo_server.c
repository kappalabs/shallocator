
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../shalloc.h"
#include "../net_utils.h"


void processor(void *data) {
	char *text = (char *)data;
	printf("server processor was given: text = >>%s<<\n", text);
	strcpy(text, "Data were processed on the server side");
	printf("server processor returning back: text = >>%s<<\n", text);
}

int main(int argcchar, char **argv) {
	shwap_server(4242, processor);
	return 0;
}
