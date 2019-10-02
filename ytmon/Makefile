all: ytmon-dl

ytmon-dl: ytmon-dl.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

clean:
	rm -f ytmon-dl

install:
	mkdir -p $(DESTDIR)/usr/bin/
	cp -v ytmon-dl $(DESTDIR)/usr/bin/

