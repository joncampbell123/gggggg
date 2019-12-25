all: download-infowars-video

download-infowars-video: download-infowars-video.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11 -ljson11

clean:
	rm -f download-infowars-video

install: download-infowars-video
	mkdir -p $(DESTDIR)/usr/bin/
	cp -vf download-infowars-video $(DESTDIR)/usr/bin/

