all: raincoat

raincoat: raincoat.c BootFlash.c boot.h BootFlash.h
	gcc -static -O2 -Wall -Werror -o raincoat raincoat.c BootFlash.c

clean:
	rm -f *.o
	rm -f raincoat
	rm -f *~
	
install:
	cp raincoat.conf $(DESTDIR)/etc/
	cp raincoat $(DESTDIR)/usr/bin/
