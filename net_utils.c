
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include "net_utils.h"
#include "shalloc.h"
#include "utils.h"


/**
 *  This structure will contain whole config file
 */
static struct clients *config;


/**
 *  Item in config file has format host#port, host is IP adress port is number in decimal format.
 *  Every item is on its own line.
 */
struct clients *read_config(char *filename) {
	int fd;
	if ((fd = open(filename, O_RDONLY)) == -1) {
		perror("open");
		return NULL;
	}

	char *line, *save_ptr;
	/* Init structures where data will be stored */
	struct clients *cls; 
	if ((cls = (struct clients *) shalloc(sizeof(struct clients))) == NULL) {
		perror("shalloc");
		/* Error check? */
		close(fd);
		return NULL;
	}
	cls->num_items = 0;
	cls->items = NULL;

	int i = 0;
	while ((line = read_line(fd)) != NULL) {
		/* Enlarge the space, if we need to */
		cls->num_items++;
		if ((cls->items = (struct client *) reshcalloc(cls->items, cls->num_items * sizeof(struct client))) == NULL) {
			perror("reshalloc");
			/* Error check? */
			close(fd);
			return NULL;
		}
		char *pos;
		if ((pos = strchr(line, '#')) == NULL) {
			/* Config file has incorrect structure */
			shee(line);
			return NULL;
		}
		cls->items[i].host = strtok_r(line, "#", &save_ptr);
		cls->items[i].port = strtok_r(NULL, "#", &save_ptr);
		i++;
	}
	if (close(fd) != 0) {
		perror("close");
	}

	return cls;
}

void free_config(struct clients *config) {
	int i;
	for (i=0; i < config->num_items; i++) {
		shee(config->items[i].host);
		config->items[i].host = NULL;
	}
	shee(config->items);
	config->items = NULL;
	shee(config);
	config = NULL;
}

int create_serv(struct clients *cls) {
	int * l_socks;
	if ((l_socks = shalloc(cls->num_items)) == NULL) {
		perror("shalloc");
		return -1;
	}

	struct addrinfo hints, *sa, *orig_sa;
	bzero(&hints, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	int err;

	int i;
	for (i=0; i < cls->num_items; i++) {
		if ((err = getaddrinfo(cls->items[i].host, cls->items[i].port, &hints, &orig_sa)) != 0) {
			fprintf(stderr, "%s\n", gai_strerror(err));
		}

		for (sa = orig_sa; sa != NULL; sa = sa->ai_next) {
			l_socks[i] = socket(sa->ai_family, sa->ai_socktype, sa->ai_protocol);
			if (l_socks[i] == -1){
				perror("socket");
				continue;
			}
			if (!connect(l_socks[i], sa->ai_addr, sa->ai_addrlen)) {
				break;
			}
		}
		if (sa ==NULL){
			int j;
			for (j=0; j <= i; j++) {
				close(l_socks[j]);
			}
			//err(1, "Exitting because of errors above");
		}
		freeaddrinfo(orig_sa);
	}

	return 0;
}

/**
 *  Send message to clients
 */
void *shwapoff(void *ptr) {
	if (config == NULL) {
		config = read_config("demo.cfg");
	}
	/*TODO priprav informace o bloku  */
	/*TODO najdi klienta */
	/*TODO odesli data  */

	return NULL;
}

void dump_config(struct clients *config) {
	int i;
	for (i=0; i < config->num_items; i++) {
		printf("%d. host: %s; port: %s\n", i+1, config->items[i].host, config->items[i].port);
	}
}
