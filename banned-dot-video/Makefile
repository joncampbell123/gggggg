all: get-channel-live-url

get-channel-live-url: get-channel-live-url.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

clean:
	rm -f get-channel-live-url

install: get-channel-live-url
	mkdir -p $(DESTDIR)/usr/bin/
	cp -vf get-channel-live-url $(DESTDIR)/usr/bin/

