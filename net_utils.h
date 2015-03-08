
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
 *  Shwap protocol
 *  ==============
 *
 *  Shwap client    |    Shwap server
 *  ---------------------------------
 *  shwappoff():
 *  RFS(GID, pow)  -->  % 'pow' is 'k_size' property of block, 'GID' is unique for this block
 *                 <--  (NO)READY  % if server (un)successfully made block of size 2^power bytes
 *  data           -->  % stream of data, only data section from Shallocator block is sent
 *                 <--  (NO)OK  % server (un)successfully saved all data
 *      <----------------------->
 *  shwapon():
 *  RFD(RID)       -->  % based on unique 'RID' number, server containing this block will respond...
 *                 <--  (NO)READY  % ...if this client has the requested data block available
 *  (NO)OK         -->  % server is (not)prepared to read the data block (based on shalloc() value)
 *                 <--  data  % data itself
 *
 */


/**
 *  SHWAP PROTOCOL COMMANDS
 *  -----------------------
 *  First two bytes define type of command.
 */
#define CMD_LEN	2
/**
 *  Request for space:
 *  Third section 'ID' is of type uint32_t, therefore 4B long.
 *  Fourth section 'power' has length one byte.
 *  |S|R|ID|power|
 */
#define RFS	"SR"
/**
 *  Request for data:
 *  Third section 'ID' is of type uint32_t, therefore 4B long.
 *  |D|R|ID|
 */
#define RFD	"DR"
/* These commands don't include any parameters */
#define OK	"OK"
#define NOOK	"KO"
#define READY	"RE"
#define NOREADY	"NR"


/**
 *  Read given config file and prepare configs structure for all the items in this file.
 *  Returns NULL on error, error message will be printed out.
 *  Otherwise it returns pointer to prepared structure.
 */
extern struct clients *read_config(char *filename);

/**
 *  Take given config file and free all alocated memory properly.
 */
extern void free_config(struct clients *);

/**
 *  Takes pointer to data segment of Shallocator block.
 *  Then it tries to send this block to first available Shwap server.
 *  Returned pointer is either NULL if process is unsuccessful, or in other cases this pointer
 *  is going to be used to retrieve data back by passing it into shwapon() function.
 */
extern void *shwapoff(void *ptr);

/**
 *  Retrieve block back from Shwap server. Block is identified by 'shwap_ptr' which was
 *  returned as a result from previous call to shwapoff().
 *  Returns pointer to retrieved data, NULL if operation was unsuccessful.
 */
extern void *shwapon(void *shwap_ptr);

/**
 *  Act as Shwap server on given port. Listen for requests and try to satisfy them.
 *  If 'p_func' is not NULL, it will be performed on every fully retrieved data block.
 *
 *  TODO this function will be broken and performed by threads
 */
extern void shwap_server(int port, void (*p_func)(void *));

/**
 *  Print out parsed configuration.
 */
extern void dump_config(struct clients *config);

#endif
