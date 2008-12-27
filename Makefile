prefix = $(HOME)/local
exec_prefix = $(prefix)
libdir = $(prefix)/lib
includedir = $(prefix)/include
pkgconfigdir = $(libdir)/pkgconfig

GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS = $(shell pkg-config --libs glib-2.0)

UNIT_TEST = -DFK_UNIT_TEST

all: libfk.so

fk.o: fk.c fk.h
	gcc -c -g -O0 $(UNIT_TEST) $(GLIB_CFLAGS) $(GLIB_LIBS) $<

test: test.c libfk.so
	gcc -g -O0 $(UNIT_TEST) $(GLIB_CFLAGS) $(GLIB_LIBS) -o $@ $<
	./$@
	-rm -f *.png

libfk.so: fk.c
	gcc -shared -fPIC -Os -o $@ $(GLIB_CFLAGS) $(GLIB_LIBS) $^

ctags:
	find . -name '*.[ch]' | ctags -L -

libfk.pc: libfk.pc.m4
	m4 -DLIBDIR=$(libdir) -DINCLUDEDIR=$(includedir) $< > $@

install: libfk.so fk.h libfk.pc
	mkdir -p $(libdir) $(includedir)/libfk $(pkgconfigdir)
	strip -s libfk.so
	install -m 755 libfk.so $(libdir)/libfk.so
	install -m 644 fk.h $(includedir)/libfk/fk.h
	install -m 644 libfk.pc $(pkgconfigdir)/libfk.pc

uninstall:
	-rm -f $(libdir)/libfk.so

clean:
	-rm -f fk test *.o *.png *.so libfk.pc

.PHONY: all clean ctags install uninstall
