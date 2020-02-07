all: ytmon-dl ytmon-update-pl

ytmon-dl: ytmon-dl.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

ytmon-update-pl: ytmon-update-pl.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

clean:
	rm -f ytmon-dl ytmon-update-pl

install: ytmon-dl ytmon-update-pl
	mkdir -p $(DESTDIR)/usr/bin/
	cp -vf ytmon-dl ytmon-update-pl $(DESTDIR)/usr/bin/

