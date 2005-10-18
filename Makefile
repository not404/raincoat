CC?=		cc
CFLAGS?=	-O -pipe

all: raincoat

raincoat: src/raincoat.c src/BootFlash.c src/boot.h src/BootFlash.h
	mkdir -p bin
	$(CC) -static -Wall -Werror $(CFLAGS) -o ./bin/raincoat src/raincoat.c src/BootFlash.c

clean:
	rm -f src/*.o
	rm -f bin/raincoat
	rm -f src/*~
	rm -f *~
	
install:
	cp ./etc/raincoat.conf $(DESTDIR)/etc/
	cp ./bin/raincoat $(DESTDIR)/usr/bin/
	chmod 755 $(DESTDIR)/usr/bin/raincoat
