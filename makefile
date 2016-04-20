PREFIX = /usr/local
MANPREFIX = $(PREFIX)/man

all: cidr

cidr: cidr.c

install: cidr cidr.1
	cp cidr $(DESTDIR)$(PREFIX)/bin/cidr
	chmod 755 $(DESTDIR)$(PREFIX)/bin/cidr
	cp cidr.1 $(DESTDIR)$(MANPREFIX)/man1/cidr.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/cidr.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cidr
	rm -f $(DESTDIR)$(MANPREFIX)/man1/cidr.1

clean:
	rm -f cidr

.PHONY: all install uninstall clean
