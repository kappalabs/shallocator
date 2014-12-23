#ifndef SHALLOC_H_
#define SHALLOC_H_


/**
 *  Allocate 'size' bytes and return pointer to the allocated memory.
 *  The memory is not initialized. If 'size' is 0, shalloc() returns NULL.
 *  NULL is also returned on error.
 */
extern void *shalloc(size_t size);

/**
 *  Allocate space for 'nmemb' objects of 'size' bytes and return pointer to the
 *  allocated memory. The memory is initialized to zero. If either 'nmemb' or
 *  'size' is zero, shcalloc() returns NULL.
 *  NULL is also returned on error.
 */
extern void *shcalloc(size_t nmemb, size_t size);

/**
 *  Change the size of the memory block pointed to by 'ptr' to 'size' bytes.
 *  The previous content will be unchanged, added memory will not be initialized.
 *  If 'ptr' is NULL, then the call is equivalent to shalloc(size).
 *  If 'size' is zero, then the call is equivalent to free(ptr).
 *  Pointer 'ptr' must have been returned by calling another shallocator
 *  allocation function or it is NULL.
 *  Returns pointer to the memory block of required size with data unchanged,
 *  on error, or if size was zero, NULL is returned.
 */
extern void *reshalloc(void *ptr, size_t size);

/**
 *  Mostly the same as reshalloc(). The previous content will be unchanged,
 *  in the contrast with reshalloc(), added memory will be initialized with zeros.
 */
extern void *reshcalloc(void *ptr, size_t size);

/**
 *  Function frees the memory at position 'ptr', which is either NULL or pointer
 *  which was obtained from some shallocator allocation function.
 *  If 'ptr' is NULL, no operation is performed.
 *  If shee() is called improperly, its behavior is undefined.
 */
extern void shee(void *ptr);


/**
 *  Dumps heap into local file.
 */
extern int dump_mem(/*TODO*/);

/**
 *  Start to share the heap memory by creating TCP connection with hosts
 *  defined in local config file.
 *  Return number of hosts available, -1 on error.
 */
extern int share(/*TODO*/);

/**
 *  Take the heap memory and swap it off onto hosts from local config file.
 *  Return number of hosts, that retrieved the data, -1 on error.
 */
extern int shwapoff(/*TODO*/);

/**
 *  Make the heap memory invisible again by closing all connections with hosts.
 */
extern int unshare(/*TODO*/);

/**
 *  As a host, accept connection and start retrieving the shared memmory.
 */
extern int accept_shared(/*TODO*/);


#endif
