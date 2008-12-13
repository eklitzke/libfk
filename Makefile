GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS = $(shell pkg-config --libs glib-2.0)

fk: fk.c
	gcc -g -o fk $(GLIB_CFLAGS) $(GLIB_LIBS) fk.c

clean:
	-rm -f fk *.o

.PHONY: clean
