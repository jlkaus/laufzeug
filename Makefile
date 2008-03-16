laufzeug:  laufzeug.c
	gcc -o $@ $< -ldiscid -DNOLIBDVDREAD

dvdinfo:  laufzeug.c
	gcc -o $@ $< -ldvdread -DNOLIBDISCID

ldiscid: laufzeug.c
	gcc -o $@ $< -DNOLIBDVDREAD -DNOLIBDISCID

all: laufzeug dvdinfo ldiscid

install:
	cp dvdinfo /usr/local/bin/dvdinfo


.PHONY: clean

clean:
	rm -f laufzeug dvdinfo ldiscid
