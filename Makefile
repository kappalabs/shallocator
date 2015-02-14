.PHONY: clean

CFLAGS	= -Wall -c -g #-m32 #-std=c89 #-m64
LDFLAGS	= -Wall -g #-m32 #-m64
LDLIBS	= -lm
PROG	= shalloc
OBJS	= shalloc.o utils.o
CC	= gcc


all:	$(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)
