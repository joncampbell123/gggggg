all: get-banned-channel-live-url download-banned-video

download-banned-video: download-banned-video.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

get-banned-channel-live-url: get-banned-channel-live-url.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

clean:
	rm -f get-banned-channel-live-url download-banned-video

install: get-banned-channel-live-url download-banned-video
	mkdir -p $(DESTDIR)/usr/bin/
	cp -vf download-banned-video $(DESTDIR)/usr/bin/
	cp -vf get-banned-channel-live-url $(DESTDIR)/usr/bin/

