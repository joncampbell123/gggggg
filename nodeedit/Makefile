all: nodeedit

nodeedit: nodeedit.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean:
	rm -f nodeedit

install:
	mkdir -p $(DESTDIR)/usr/bin/
	cp -v nodeedit $(DESTDIR)/usr/bin/

