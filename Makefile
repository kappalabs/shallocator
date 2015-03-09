.PHONY: clean examples

CFLAGS	= -Wall -c -g# -m32 #-std=c89 #-m64
LDFLAGS	= -Wall -g# -m32 #-m64
LDLIBS	= -lm
LIB	= libshalloc.a
SOURCES	= $(wildcard *.c)
OBJS	= $(SOURCES:.c=.o)
CC	= gcc


all:	libobj libshalloc examples

libobj: $(wildcard *.c) $(wildcard *.h)
	$(CC) $(LDFLAGS) -c $(SOURCES) $(LDLIBS)

libshalloc: $(LIB)
	ar -cvq $^ $(OBJS)

examples: $(LIB)
	cd examples; make

#$(PROG): $(OBJS)
#	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#
#%.o: %.c %.h
#	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)
