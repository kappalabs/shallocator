
#ifndef NET_UTILS_H_
#define NET_UTILS_H_

#include "shalloc.h"

struct client {
	char *host;
	char *port;
};

struct clients {
	int num_items;
	struct client *items;
};


/**
 *  Read given config file and prepare configs structure for all the items in this file.
 *  Returns NULL on error, error message will be printed out.
 */
extern struct clients *read_conf(char *filename);

/**
 *  Take given config file and free all alocated memory properly.
 */
extern void free_config(struct clients *);

/**
 * Create server and return number of established connections 
 */
extern int create_serv(struct clients *);

/**
 *  Establish connection to the server, and listen for incomming blocks
 */
extern int connect_serv(char *server);

/**
 *  Store given block to random available client
 */
extern int shwapoff(struct client *, struct block *);

/**
 *  Retrieve block back from client
 */
extern struct block *shwapon();

extern void dump_config(struct clients *config);

#endif
