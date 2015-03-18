.PHONY: clean

CFLAGS	:= -Wall -g -O3
CC	:= gcc
SOURCES	:= $(wildcard *.c)
DEMOS	:= $(basename $(SOURCES))
RM	:= rm -f
LIBNAME	:= shalloc


all: $(DEMOS)

demo_client: demo_client.c
	$(CC) $(CFLAGS) $@.c -o $@ -L.. -l$(LIBNAME)

run_client: demo_client
	LD_LIBRARY_PATH=../ ./$^

demo_server: demo_server.c
	$(CC) $(CFLAGS) $@.c -o $@ -L.. -l$(LIBNAME)

run_server: demo_server
	LD_LIBRARY_PATH=../ ./$^

demo_shalloc: demo_shalloc.c
	$(CC) $(CFLAGS) $@.c -o $@ -L.. -l$(LIBNAME)

run_shalloc: demo_shalloc
	LD_LIBRARY_PATH=../ ./$^

clean:
	$(RM) $(DEMOS) *.o *.so* *.a