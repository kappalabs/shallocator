.PHONY: clean

CFLAGS	= -Wall -c #-m64
LDFLAGS	= -Wall #-m64
LDLIBS	= -lm
PROG	= shalloc
OBJS	= shalloc.o #next.o
CC	= gcc


all:	$(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)
