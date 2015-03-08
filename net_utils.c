
#include <inttypes.h>
#include <string.h>
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
#include <arpa/inet.h>

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
	return 0;
}

/**
 *  Send data block to random server.
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

/**
 *  Retrieve data block back.
 */
void *shwapon(void *shwap_ptr) {
	/* False call */
	if (config == NULL || shwap_ptr == NULL) {
		return NULL;
	}

	return NULL;
}

/**
 *  Read-write wrapper, tries to process whole given string instead of giving up.
 *  Returns -1 on error, otherwise total number of bytes readed/writed which should
 *  always be equal to 'size'.
 */
ssize_t net_read_write(int sock, char *buf, size_t size, int is_read) {
	ssize_t total = 0;
	ssize_t n;
	if (is_read) {
		bzero(buf, size);
	}
	while (total < size) {
		if (is_read) {
			n = read(sock, (char *)((size_t)buf + total), size - total);
		} else {
			n = write(sock, (char *)((size_t)buf + total), size - total);
		}
		total += n;
		if (n == -1) {
			perror("read");
			return -1;
		} else if (is_read && n == 0 && total < size) {
			/* Connection was probably lost */
			return -1;
		}
	}
	return total;
}

/**
 *  Read/write exactly 'size' bytes if possible into/from 'buf' from/into socket 'sock'.
 */
ssize_t net_read(int sock, char *buf, size_t size) {
	return net_read_write(sock, buf, size, 1);
}
ssize_t net_write(int sock, char *buf, size_t size) {
	return net_read_write(sock, buf, size, 0);
}

/**
 *  Read first two bytes into given string.
 *  On error, set 'com' to empty string.
 */
void read_command(int sock, char *com) {
	bzero(com, CMD_LEN);
	if (net_read(sock, com, CMD_LEN) < 0) {
		bzero(com, CMD_LEN);
	}
}

void shwap_client(int port, void (*p_func)(void *)) {
	int sockfd, cl_sock;
	socklen_t cli_len;
	struct sockaddr_in serv_addr, cli_addr;

	/* Connection will be realized through TCP */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("ERROR opening socket");
		return;
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (INADDR_ANY) {
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		return;
	}
	listen(sockfd, 5);
	cli_len = sizeof(cli_addr);

while(1) {

	/* Start listening for incomming conection */
	cl_sock = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
	if (cl_sock < 0) {
		perror("ERROR on accept");
		return;
	}

	/* Start reading */
	char command[CMD_LEN];
	uint8_t power;
	uint32_t GID, RID; /* Given and requested IDs */
	uint32_t tmp;

	/* Accept RFS(power, ID) */
	read_command(cl_sock, command);
	if (strncmp(RFS, command, CMD_LEN) != 0) {
		goto cleanup_fd;
	}
	LOG("Incomming command: RFS\n");
	/* Read parameters of this command */
	if (net_read(cl_sock, (char *)&tmp, sizeof(tmp)) < 0) {
		goto cleanup_fd;
	}
	GID = ntohl(tmp);
	if (net_read(cl_sock, (char *)&power, 1) < 0) {
		goto cleanup_fd;
	}
	LOG("RFS(GID=%"PRIu32", power=%u)\n", GID, power);

	/* Try to shalloc() requested space */
	void *ptr;
	unsigned long bl_size = pow2(power) - B_SIZE;
	ptr = shalloc(bl_size);
	/* Return possibility to allocate this amount of space */
	if (ptr == NULL) {
		net_write(cl_sock, NOREADY, CMD_LEN);
		LOG("NOREADY sended\n");
		goto cleanup_fd;
	}
	net_write(cl_sock, READY, CMD_LEN);
	LOG("READY sended\n");

	/* Receive data and store them + confirm delivery */
	//TODO buffered_net_read()
	if (net_read(cl_sock, ptr, bl_size) == bl_size) {
		net_write(cl_sock, OK, CMD_LEN);
		LOG("All data received and stored\n");
	} else {
		net_write(cl_sock, NOOK, CMD_LEN);
		LOG("Some data were probably lost\n");
		goto cleanup_fd;
	}

	/* Perform desired function on it */
	if (p_func != NULL) {
		LOG("desired function will be applied\n");
		p_func(ptr);
		LOG("desired function was applied\n");
	}

	/* Wait for shwapon() request ~ RFD command */
	read_command(cl_sock, command);
	if (strncmp(RFD, command, CMD_LEN) != 0) {
		goto cleanup_ptr;
	}
	LOG("Incomming command: RFD\n");
	/* Read parameters of this command */
	if (net_read(cl_sock, (char *)&tmp, sizeof(tmp)) < 0) {
		goto cleanup_ptr;
	}
	RID = ntohl(tmp);
	LOG("RFD(RID=%"PRIu32")\n", RID);

	/* TODO Verify existence of RID in local memory */
	// => ziskej a nastav 'ptr' na pozadovany blok, pokud existuje + oprava nasledujiciho if
	/* Send response based on verification */
	if (GID == RID) {
		net_write(cl_sock, READY, CMD_LEN);
	} else {
		net_write(cl_sock, NOREADY, CMD_LEN);
		// 'ptr' blok si chci v pameti nechat, nekdo jiny ho AC bude chtit
		goto cleanup_fd;
	}

	/* Read response of client - if he's ready to recieve data */
	read_command(cl_sock, command);
	if (strncmp(OK, command, CMD_LEN) != 0) {
		/* Server is not ready, he can request again after he will have enough space */
		goto cleanup_ptr;
	}
	LOG("Incomming command: OK\n");

	/* Now we can send data to client */
	//TODO buffered_net_write()
	if (net_write(cl_sock, ptr, bl_size) == bl_size) {
		LOG("All data send\n");
	} else { //TODO toto by se MP osetrovat ani nemelo
		LOG("Some data were probably lost while sending\n");
		/* Let's get another chance to client */
		goto cleanup_fd;
	}

	/* This session ended */
	cleanup_ptr:
		shee(ptr);
	cleanup_fd:
		close(cl_sock);

	} //TODO EO while(true) loop
	close(sockfd);

	return;
}

void dump_config(struct clients *config) {
	int i;
	for (i=0; i < config->num_items; i++) {
		printf("%d. host: %s; port: %s\n", i+1, config->items[i].host, config->items[i].port);
	}
}
