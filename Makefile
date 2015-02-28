.PHONY: clean

CFLAGS	= -Wall -c -g #-m32 #-std=c89 #-m64
LDFLAGS	= -Wall -g #-m32 #-m64
LDLIBS	= -lm
PROG	= shalloc
SOURCES	= $(wildcard *.c)
OBJS	= $(SOURCES:.c=.o)
CC	= gcc


all:	$(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)
