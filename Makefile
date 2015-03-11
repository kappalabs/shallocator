.PHONY: clean examples

CFLAGS	= -Wall -g -c# -m32 #-std=c89 #-m64
LDFLAGS	= -Wall -g# -m32 #-m64
LDLIBS	= -lm
LIB	= libshalloc.a
SOURCES	= $(wildcard *.c)
OBJS	= $(SOURCES:.c=.o)
CC	= gcc


all:	libshalloc examples

#libobj: $(wildcard *.c) $(wildcard *.h)
#	$(CC) $(CFLAGS) $(SOURCES) $(LDLIBS)

libshalloc: $(OBJS)
#	ar -cvq $(LIB) $(OBJS)
	ar -rcs $(LIB) $(OBJS)

examples: $(LIB)
	cd examples; make

#$(PROG): $(OBJS)
#	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#
%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)
