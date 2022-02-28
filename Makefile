.PHONY: all clean

all: laufzeug dvdinfo discinfo ldiscid

laufzeug:  discinfo.c
	gcc -o $@ $< -ldiscid -DNOLIBDVDREAD

dvdinfo:  discinfo.c
	gcc -o $@ $< -ldvdread -DNOLIBDISCID

discinfo:  discinfo.c
	gcc -o $@ $< -ldiscid -ldvdread

ldiscid: discinfo.c
	gcc -o $@ $< -DNOLIBDVDREAD -DNOLIBDISCID

install: dvdinfo
	install -t /usr/local/bin/dvdinfo dvdinfo

clean:
	rm -f laufzeug discinfo dvdinfo ldiscid
