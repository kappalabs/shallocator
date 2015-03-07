
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
extern struct clients *read_config(char *filename);

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
 *  Takes pointer to data segment of Shallocator block. Then it tries to send this block to first available client.
 *  Returned pointer is either NULL if process is unsucesfull, or in other cases this pointer
 *  is going to be used to retrieve data back.
 */
extern void *shwapoff(void *ptr);

/**
 *  Retrieve block back from client. Block is identified by 'swp_bl' which was returned as a result
 *  from previous call to shwapoff().
 *  Returns pointer to retrieved data, NULL if operation was unsucesfull
 */
extern void *shwapon(void *shwap_ptr);

/**
 *  Act as Shwap client on given port. Listen for any request and try to satisfy it.
 */
extern int cl_listen(int port);

/**
 *  Protocol
 *
 *  Main program    |    Shwap client
 *  --------------------------------
 *  shwappoff():
 *  RFS(power, ID) -->  % power = log(size of block), ID is unique number for this special block
 *                 <--  (No)Ready  % if client sucesfully made block for 2^power bytes
 *  data           -->  % data itself, only data section from Shallocator block is sent
 *                 <--  OK
 *
 *  shwapon():
 *  RFD(ID)        -->  % based on unique number client with this number will respond
 *                 <--  Ready  % if client has the data available
 *  OK             -->  % if server is prepared to read the data block (he will first do shalloc())
 *                 <--  data  % data itself
 *
 */

/**
 *  Print out parsed configuration
 */
extern void dump_config(struct clients *config);

#endif
