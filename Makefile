all: raincoat

raincoat: src/raincoat.c src/BootFlash.c src/boot.h src/BootFlash.h
	mkdir bin
	gcc -static -O2 -Wall -Werror -o ./bin/raincoat src/raincoat.c src/BootFlash.c

clean:
	rm -f src/*.o
	rm -rf bin
	rm -f src/*~
	rm -f *~
	
install:
	cp ./etc/raincoat.conf $(DESTDIR)/etc/
	cp ./bin/raincoat $(DESTDIR)/usr/bin/
