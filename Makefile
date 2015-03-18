.PHONY: clean examples

CFLAGS	:= -Wall -g -O3 -fPIC
CC	:= gcc
SOURCES	:= $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o)
LIBNAME	:= shalloc
MAJOR	:= 0
MINOR	:= 1
VERSION	:= $(MAJOR).$(MINOR)
RM	:= rm -f

all: lib$(LIBNAME).so.$(VERSION) examples

lib$(LIBNAME).so.$(VERSION): $(OBJS)
#	$(CC) -shared -Wl,-soname,lib$(LIBNAME).so.$(MAJOR) $^ -o $@
	$(CC) -shared -o $@ $^ -lm -pthread

lib$(NAME).so: lib$(LIBNAME).so.$(VERSION)
	ldconfig -v -n .
	ln -f -s lib$(LIBNAME).so.$(VERSION) lib$(LIBNAME).so

examples: lib$(NAME).so
	cd examples; make clean; make

clean:
	$(RM) $(PROG) *.o *.so* *.a
	cd examples; make clean
