all: archive-dl-gen

archive-dl-gen: archive-dl-gen.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean:
	rm -f archive-dl-gen

install:
	mkdir -p $(DESTDIR)/usr/bin/
	cp -v archive-dl-gen $(DESTDIR)/usr/bin/

