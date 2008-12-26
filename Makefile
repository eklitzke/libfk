prefix = $(HOME)
bindir = $(prefix)/bin

GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS = $(shell pkg-config --libs glib-2.0)

UNIT_TEST = -DFK_UNIT_TEST

all: fk.o test

fk.o: fk.c fk.h
	gcc -c -g -O0 $(UNIT_TEST) $(GLIB_CFLAGS) $(GLIB_LIBS) fk.c

test: test.c fk.o
	gcc -g -O0 $(GLIB_CFLAGS) $(GLIB_LIBS) -o test test.c fk.o
	./test
	-rm -f *.png

ctags:
	find . -name '*.[ch]' | ctags -L -

install: all
	install -m 644 fk.o $(bindir)/fk.o

clean:
	-rm -f fk test *.o *.png

.PHONY: all clean ctags
