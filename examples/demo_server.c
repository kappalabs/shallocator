
#include <stdio.h>
#include <errno.h>

#include "../shalloc.h"
#include "../net_utils.h"


void processor(void *data) {
	printf("server processor: text = >>%s<<\n", (char *)data);
}

int main(int argcchar, char **argv) {
	shwap_server(4242, processor);
	return 0;
}
