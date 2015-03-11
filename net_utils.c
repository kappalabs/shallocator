
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

	/* Information about a block, which will be sent */
	struct block *bl;
	bl = B_HEAD(ptr);
	/* 'k_size' will be casted to uint8_t */
	if (bl->k_size > 255) {
		printf("Block is too large!\n");
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
		LOG("Zkousim %d. klienta >%s:%s<\n", i+1, cl.host, cl.port);
		int err;
		if ((err = getaddrinfo(cl.host, cl.port, &hints, &orig_sa)) != 0) {
			printf("%s\n", gai_strerror(err));
			continue;
		}
	
		int serv_sock;
		for (sa = orig_sa; sa != NULL; sa = sa->ai_next) {
			serv_sock = socket(sa->ai_family, sa->ai_socktype, sa->ai_protocol);
			if (serv_sock == -1){
				perror("socket");
				continue;
			}
			if (!connect(serv_sock, sa->ai_addr, sa->ai_addrlen)) {
				/* Success */
				break;
			}
			close(serv_sock);
		}
		/* No address succeeded, lets try another client */
		if (sa == NULL) {
			printf("Could not connect to client >%s:%s< !\n", cl.host, cl.port);
			continue;
		}
		/* Connection was created */
		freeaddrinfo(orig_sa);

		//TODO prekontrolovat
		/* Set maximum read() blocking time */
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
	
		/* Send RFS(GID, pow) to server */
		net_write(serv_sock, RFS, CMD_LEN);
		tmp = htonl(GID);
		net_write(serv_sock, (char *)&tmp, 4);
		net_write(serv_sock, (char *)&pow, 1);
		LOG("<<RFS(GID=%"PRIu32", pow=%u) sended\n", GID, pow);
	
		/* Wait for (NO)READY command */
		read_command(serv_sock, command);
		if (strncmp(READY, command, CMD_LEN) == 0) {
			LOG(">>Incomming command: READY\n");
			/* This server is willing to receive data */
		} else if (strncmp(NOREADY, command, CMD_LEN) == 0) {
			LOG(">>Incomming command: NOREADY\n");
			/* This server can't provide space for us, lets try another one */
			goto cleanup_fd;
		} else {
			/* Unexpected command, lets try another server */
			LOG(">>Unexpected command instead of (NO)READY!\n");
			goto cleanup_fd;
		}
	
		/* Send data to server */
		//TODO buffered_net_write()
		if (net_write(serv_sock, ptr, data_size) == data_size) {
			LOG("<<All data sent\n");
		} else { //TODO toto by se MP osetrovat ani nemelo
			LOG("<<Some data were probably lost while sending\n");
			goto cleanup_fd;
		}
	
		/* Read confirmation of received data */
		read_command(serv_sock, command);
		if (strncmp(OK, command, CMD_LEN) == 0) {
			LOG(">>Incomming command: OK\n");
		} else if (strncmp(NOOK, command, CMD_LEN) == 0) {
			/* Server is not ready, he can request again after he will have enough space */
			LOG(">>Incomming command: NOOK\n");
			goto cleanup_fd;
		} else {
			/* Unexpected command */
			LOG(">>Unexpected command instead of (NO)OK!\n");
			goto cleanup_fd;
		}

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
	//TODO kam s tim?
	free_config(config);
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
		LOG("Zkousim %d. klienta >%s:%s<\n", i+1, cl.host, cl.port);
		int err;
		if ((err = getaddrinfo(cl.host, cl.port, &hints, &orig_sa)) != 0) {
			printf("%s\n", gai_strerror(err));
			continue;
		}
	
		int serv_sock;
		for (sa = orig_sa; sa != NULL; sa = sa->ai_next) {
			serv_sock = socket(sa->ai_family, sa->ai_socktype, sa->ai_protocol);
			if (serv_sock == -1){
				perror("socket");
				continue;
			}
			if (!connect(serv_sock, sa->ai_addr, sa->ai_addrlen)) {
				/* Success */
				break;
			}
			close(serv_sock);
		}
		/* No address succeeded, lets try another client */
		if (sa == NULL) {
			printf("Could not connect to %d. client!\n", i+1);
			continue;
		}
		/* Connection successful */
		LOG("Connection succeeded\n");
		freeaddrinfo(orig_sa);
	
		/* Send RFD(RID, pow) to server */
		net_write(serv_sock, RFD, CMD_LEN);
		tmp = htonl(RID);
		net_write(serv_sock, (char *)&tmp, 4);
		net_write(serv_sock, (char *)&pow, 1);
		LOG("<<RFD(RID=%"PRIu32", pow=%u) sended\n", RID, pow);
	
		/* Wait for (NO)READY command */
		read_command(serv_sock, command);
		if (strncmp(READY, command, CMD_LEN) == 0) {
			LOG(">>Incomming command: READY\n");
			/* This server is willing to receive data */
		} else if (strncmp(NOREADY, command, CMD_LEN) == 0) {
			LOG(">>Incomming command: NOREADY\n");
			/* This server can't provide space for us, lets try another one */
			goto cleanup_fd;
		} else {
			/* Unexpected command, lets try another server */
			LOG(">>Unexpected command instead of (NO)READY!\n");
			goto cleanup_fd;
		}
	
		/* Send (NO)OK, based on result from shalloc() to server */
		void *ptr = shalloc(data_size);
		if (ptr == NULL) {
			net_write(serv_sock, NOOK, CMD_LEN);
			LOG("<<NOOK sended, shalloc() failed!\n");
			goto cleanup_fd;
		}
		net_write(serv_sock, OK, CMD_LEN);
		LOG("<<OK sended, shalloc() successful\n");

		/* Recieve data from server */
		//TODO buffered_net_read()
		if (net_read(serv_sock, ptr, data_size) == data_size) {
			LOG(">>All requested data were received\n");
		} else { //TODO osetrovat?
			LOG(">>Some data were probably lost while receiving!\n");
			goto cleanup_fd;
		}
	
		/* Cleanup */
		if (close(serv_sock) == -1) {
			perror("close");
		}

		/* Swapon was successful, ID can be reused */
		shee(ret_bl);
		ret_bl = NULL;
		//TODO: pokud je ID set prazdny, zavolam
		//free_config(config);

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

static char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
	switch(sa->sa_family) {
		case AF_INET:
			inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
			break;
		case AF_INET6:
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
			break;
		default:
			strncpy(s, "Unknown AF", maxlen);
			return NULL;
	}
	return s;
}

//TODO
void *first;
uint32_t first_ID;
void set_add(uint32_t ID, void *ptr) {
	LOG("set_add() is WIP!\n");
	first = ptr;
	first_ID = ID;
}
void *set_find(uint32_t ID) {
	LOG("set_find() is WIP!\n");
	if (first_ID == ID) {
		return first;
	}
	LOG("set_find() ERROR!\n");
	return NULL;
}
void set_remove(uint32_t ID) {
	LOG("set_remove() is WIP!\n");
}
//TODO

static void rfs_command(int cl_sock, void (*p_func)(void *)) {
	uint8_t pow;
	uint32_t GID, tmp;

	/* Read parameters of this command */
	if (net_read(cl_sock, (char *)&tmp, sizeof(tmp)) < 0) {
		return;
	}
	GID = ntohl(tmp);
	if (net_read(cl_sock, (char *)&pow, 1) < 0) {
		return;
	}
	LOG(">>Incomming command: RFS(GID=%"PRIu32", pow=%u)\n", GID, pow);

	/* Try to shalloc() requested space */
	void *ptr;
	unsigned long bl_size = pow2(pow) - B_SIZE;
	//TODO + 1 - smazano, jeste je potreba osetrit
	ptr = shalloc(bl_size);
	/* Return possibility to allocate this amount of space */
	if (ptr == NULL) {
		net_write(cl_sock, NOREADY, CMD_LEN);
		LOG("<<NOREADY sended\n");
		return;
	}
	net_write(cl_sock, READY, CMD_LEN);
	LOG("<<READY sended\n");

	/* Receive data and store them + confirm delivery */
	//TODO buffered_net_read()
	if (net_read(cl_sock, ptr, bl_size) == bl_size) {
		net_write(cl_sock, OK, CMD_LEN);
		LOG("<<All data received and stored\n");
	} else {
		net_write(cl_sock, NOOK, CMD_LEN);
		LOG("<<Some data were probably lost\n");
		return;
	}

	/* Save information about this block */
	set_add(GID, ptr);

	/* Perform desired function on it */
	if (p_func != NULL) {
		LOG("Processor function will be applied now\n");
		p_func(ptr);
	}
}

static void rfd_command(int cl_sock) {
	char command[CMD_LEN];
	uint8_t pow;
	uint32_t RID, tmp;
	void *ptr;

	/* Read parameters of this command */
	if (net_read(cl_sock, (char *)&tmp, sizeof(tmp)) < 0) {
		return;
	}
	RID = ntohl(tmp);
	if (net_read(cl_sock, (char *)&pow, 1) < 0) {
		return;
	}
	LOG(">>Incomming command: RFD(RID=%"PRIu32", pow=%u)\n", RID, pow);

	//TODO
	/* Verify existence of RID in local memory, then send response based on verification */
	if ((ptr = set_find(RID)) != NULL) {
		net_write(cl_sock, READY, CMD_LEN);
		LOG("<<READY sended\n");
	} else {
		net_write(cl_sock, NOREADY, CMD_LEN);
		LOG("<<NOREADY sended\n");
		/* We don't have this block, therefore client has to ask another server */
		return;
	}

	/* Read response of client - if he's ready to recieve the data */
	read_command(cl_sock, command);
	if (strncmp(OK, command, CMD_LEN) == 0) {
		LOG(">>Incomming command: OK\n");
	} else if (strncmp(NOOK, command, CMD_LEN) == 0) {
		/* Server is not ready, he can request again after he will have enough space */
		LOG(">>Incomming command: NOOK\n");
		return;
	} else {
		/* Unexpected command */
		LOG(">>Unexpected command instead of (NO)OK!\n");
		return;
	}

	/* Size of data section in bytes, which client requested */
	size_t data_size = pow2(pow) - B_SIZE;

	/* Now we can send the data to client */
	//TODO buffered_net_write()
	if (net_write(cl_sock, ptr, data_size) == data_size) {
		LOG("<<All of the required data were sent\n");
	} else { //TODO chovani do dokumentace!
		LOG("<<Some data were probably lost while sending!\n");
		/* Let's get another chance to client and don't free this block */
		return;
	}

	/* This block is no longer needed to be stored here */
	shee(ptr);
	set_remove(RID);
}

void shwap_server(int port, void (*p_func)(void *)) {
	int sockfd, cl_sock;
	socklen_t cli_len;
	struct sockaddr_in serv_addr, cli_addr;
	char command[CMD_LEN];
	char straddr[INET6_ADDRSTRLEN];

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
		LOG("\nWAITING FOR CLIENT\n");
		/* Start listening for incomming conection */
		cl_sock = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
		if (cl_sock < 0) {
			perror("accept");
			continue;
		}
		LOG("Client >%s:%u< connected\n", get_ip_str((struct sockaddr *)&cli_addr, straddr, sizeof(straddr)), ntohs(cli_addr.sin_port));
	
		/* Communication starts now */
		read_command(cl_sock, command);
		if (strncmp(RFS, command, CMD_LEN) == 0) {
			/* Accept RFS(GID, pow) */
			rfs_command(cl_sock, p_func);
		} else if (strncmp(RFD, command, CMD_LEN) == 0) {
			/* Accept RFD(RID, pow) */
			rfd_command(cl_sock);
		} else {
			/* Unexpected command */
			LOG(">>Unexpected command!\n");
		}
	
		/* Connection with this client is over */
		if (close(cl_sock) == -1) {
			perror("close");
		}
	}
	if (close(sockfd) == -1) {
		perror("close");
	}

	return;
}

void dump_config(struct clients *config) {
	int i;
	for (i=0; i < config->num_items; i++) {
		printf("%d. host: %s; port: %s\n", i+1, config->items[i].host, config->items[i].port);
	}
}
