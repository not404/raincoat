all: raincoat

raincoat: raincoat.c BootFlash.c boot.h BootFlash.h
	gcc -O2 -Wall -Werror -o raincoat raincoat.c BootFlash.c

clean:
	rm -f *.o
	rm -f raincoat
	rm -f *~
	
install:
	cp raincoat.conf /etc/
	cp raincoat /usr/bin/