dvdinfo:  discinfo.c
	gcc -o $@ $< -ldvdread -DNOLIBDISCID

laufzeug:  discinfo.c
	gcc -o $@ $< -ldiscid -DNOLIBDVDREAD

discinfo:  discinfo.c
	gcc -o $@ $< -ldiscid -ldvdread

ldiscid: discinfo.c
	gcc -o $@ $< -DNOLIBDVDREAD -DNOLIBDISCID

all: laufzeug dvdinfo discinfo ldiscid

install: dvdinfo
	cp dvdinfo /usr/local/bin/dvdinfo


.PHONY: clean

clean:
	rm -f laufzeug discinfo dvdinfo ldiscid
