
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
 *  This structure will contain whole config file.
 */
static struct clients *config;

/**
 *  Structure for information about all swapped blocks.
 */
static struct swp_bls {
	struct swp_bl *blocks;
	uint32_t num;
} swapped_bls;


/**
 *  Item in config file has format host#port, host is IP adress port is number in decimal format.
 *  Every item is on its own line.
 */
struct clients *read_config(char *filename) {
	int serv_sock;
	if ((serv_sock = open(filename, O_RDONLY)) == -1) {
		perror("open");
		return NULL;
	}

	char *line, *save_ptr;
	/* Init structures where data will be stored */
	struct clients *cls; 
	if ((cls = (struct clients *) shalloc(sizeof(struct clients))) == NULL) {
		perror("shalloc");
		/* Error check? */
		close(serv_sock);
		return NULL;
	}
	cls->num_items = 0;
	cls->items = NULL;

	int i = 0;
	while ((line = read_line(serv_sock)) != NULL) {
		/* Enlarge the space, if we need to */
		cls->num_items++;
		if ((cls->items = (struct client *) reshcalloc(cls->items, cls->num_items * sizeof(struct client))) == NULL) {
			perror("reshalloc");
			/* Error check? */
			close(serv_sock);
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
	if (close(serv_sock) != 0) {
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

/**
 *  Send data block to random server.
 */
void *shwapoff(void *ptr) {
	if (config == NULL) {
		config = read_config("demo.cfg");
	}

	/* Information about block to be sent */
	struct block *bl;
	bl = B_HEAD(ptr);
	/* 'k_size' will be casted to uint8_t */
	if (bl->k_size > 255) {
		fprintf(stderr, "Block is too large!\n");
		return NULL;
	}
	uint8_t pow = (uint8_t)bl->k_size;
	size_t data_size = B_DATA_SIZE(bl); 
	char command[CMD_LEN];
	//TODO rozumnejsi pridelovani
	uint32_t GID = swapped_bls.num;
	uint32_t tmp;

	/* Try to find first available client */
	int i;
	for (i=0; i < config->num_items; i++) {
		struct addrinfo hints, *sa, *orig_sa;
		bzero(&hints, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_UNSPEC;
	
		struct client cl = config->items[i];
		int err;
		if ((err = getaddrinfo(cl.host, cl.port, &hints, &orig_sa)) != 0) {
			fprintf(stderr, "%s\n", gai_strerror(err));
		}
	
		int serv_sock;
		for (sa = orig_sa; sa != NULL; sa = sa->ai_next) {
			serv_sock = socket(sa->ai_family, sa->ai_socktype, sa->ai_protocol);
			if (serv_sock == -1){
				perror("socket");
				continue;
			}
			if (!connect(serv_sock, sa->ai_addr, sa->ai_addrlen)) {
				break;
			}
		}
		freeaddrinfo(orig_sa);
		/* Unable to connect, lets try another client */
		if (serv_sock == -1) {
			continue;
		}
		/* Connection successful */
	
		/* Send RFS(GID, pow) to server */
		net_write(serv_sock, RFS, CMD_LEN);
		tmp = htonl(GID);
		net_write(serv_sock, (char *)&tmp, sizeof(uint32_t));
		net_write(serv_sock, (char *)&pow, 1);
	
		/* Wait for (NO)READY command */
		read_command(serv_sock, command);
		if (strncmp(READY, command, CMD_LEN) != 0) {
			LOG("Incomming command: ? (NO)READY\n");
			goto cleanup_fd;
		}
		LOG("Incomming command: READY\n");
	
		/* Send data to server */
		//TODO buffered_net_write()
		if (net_write(serv_sock, ptr, data_size) == data_size) {
			LOG("All data sent\n");
		} else { //TODO toto by se MP osetrovat ani nemelo
			LOG("Some data were probably lost while sending\n");
			goto cleanup_fd;
		}
	
		/* Read confirmation of received data */
		read_command(serv_sock, command);
		if (strncmp(OK, command, CMD_LEN) != 0) {
			LOG("Incomming command: ? (NO)OK\n");
			goto cleanup_fd;
		}
		LOG("Incomming command: OK\n");

		/* Block is swapped out and can be safely freed now */
		shee(ptr);

		/* Cleanup */
		if (close(serv_sock) == -1) {
			perror("close");
		}

		/* Save information about this swapped block */
		//TODO jeste je treba nekde poznamenat pouzity ID
		swapped_bls.num++;
		struct swp_bl *swb;
		if ((swb = (struct swp_bl *)shalloc(sizeof(struct swp_bl))) == NULL) {
			perror("shalloc");
			return NULL;
		}
		swb->ID = GID;
		swb->pow = pow;
		return (void *)swb;
	
		/* Session with this server is over */
		cleanup_fd:
			if (close(serv_sock) == -1) {
				perror("close");
			}
	
	}
	return NULL;
}

/**
 *  Retrieve shwaped data block back from server.
 */
void *shwapon(void *shwap_ptr) {
	/* False call */
	if (config == NULL || shwap_ptr == NULL) {
		return NULL;
	}

	/* Information about block to be recieved */
	struct swp_bl *ret_bl = (struct swp_bl *)shwap_ptr;

	uint8_t pow = ret_bl->pow;
	uint32_t RID = ret_bl->ID;
	size_t data_size = pow2(pow) - B_SIZE;
	char command[CMD_LEN];
	uint32_t tmp;

	/* Try to find client with desired block */
	int i;
	for (i=0; i < config->num_items; i++) {
		struct addrinfo hints, *sa, *orig_sa;
		bzero(&hints, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_UNSPEC;
	
		struct client cl = config->items[i];
		int err;
		if ((err = getaddrinfo(cl.host, cl.port, &hints, &orig_sa)) != 0) {
			fprintf(stderr, "%s\n", gai_strerror(err));
		}
	
		int serv_sock;
		for (sa = orig_sa; sa != NULL; sa = sa->ai_next) {
			serv_sock = socket(sa->ai_family, sa->ai_socktype, sa->ai_protocol);
			if (serv_sock == -1){
				perror("socket");
				continue;
			}
			if (!connect(serv_sock, sa->ai_addr, sa->ai_addrlen)) {
				break;
			}
		}
		freeaddrinfo(orig_sa);
		/* Unable to connect, lets try another client */
		if (serv_sock == -1) {
			continue;
		}
		/* Connection successful */
	
		/* Send RFD(RID) to server */
		net_write(serv_sock, RFD, CMD_LEN);
		tmp = htonl(RID);
		net_write(serv_sock, (char *)&tmp, sizeof(uint32_t));
	
		/* Wait for (NO)READY command */
		read_command(serv_sock, command);
		if (strncmp(READY, command, CMD_LEN) != 0) {
			LOG("Incomming command: ? (NO)READY\n");
			goto cleanup_fd;
		}
		LOG("Incomming command: READY\n");
	
		/* Send (NO)OK, based on result from shalloc() to server */
		void *ptr = shalloc(data_size);
		if (ptr == NULL) {
			LOG("Sending NOOK, shalloc() failed!\n");
			net_write(serv_sock, NOOK, CMD_LEN);
			goto cleanup_fd;
		}
		LOG("Sending OK, shalloc() successful\n");
		net_write(serv_sock, OK, CMD_LEN);

		/* Recieve data from server */
		//TODO buffered_net_read()
		if (net_read(serv_sock, ptr, data_size) == data_size) {
			LOG("All data received\n");
		} else { //TODO toto by se MP osetrovat ani nemelo
			LOG("Some data were probably lost while receiving!\n");
			goto cleanup_fd;
		}
	
		/* Cleanup */
		if (close(serv_sock) == -1) {
			perror("close");
		}

		/* Swapon was successful, ID can be reused */
		shee(ret_bl);
		ret_bl = NULL;

		/* Return pointer to retrieved block data section */
		return ptr;
	
		/* Session with this server is over */
		cleanup_fd:
			if (close(serv_sock) == -1) {
				perror("close");
			}
	
	}
	return NULL;
}

void shwap_server(int port, void (*p_func)(void *)) {
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
		//TODO jinak nez s goto
		if (strncmp(RFD, command, CMD_LEN) != 0) {
			goto cleanup_fd;
		}
		goto rfd_command;
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

	//TODO neni vhodne uziti goto
	goto cleanup_fd;
	rfd_command:

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
		if(close(cl_sock) == -1) {
			perror("close");
		}

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
