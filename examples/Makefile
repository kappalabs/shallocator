.PHONY: clean

CFLAGS	= -Wall -c -g# -m32 #-std=c89 #-m64
LDFLAGS	= -Wall -g# -m32 #-m64
LDLIBS	= -lm
SOURCES	= $(wildcard *.c)
LIB	= ../libshalloc.a
DEMOS	= $(basename $(SOURCES))
CC	= gcc


all:	$(DEMOS)

demo_client:	$(LIB) demo_client.c
	$(CC) $(LDFLAGS) -o $@ $^ ../libshalloc.a
#	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

demo_server:	$(LIB) demo_server.c
	$(CC) $(LDFLAGS) -o $@ $^ ../libshalloc.a
#	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

demo_shalloc:	$(LIB) demo_shalloc.c
	$(CC) $(LDFLAGS) -o $@ $^ ../libshalloc.a
#	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(DEMOS) $(OBJS)