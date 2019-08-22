all: download-infowars-video

download-infowars-video: download-infowars-video.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs` -std=c++11

clean:
	rm -f download-infowars-video

install:
	mkdir -p $(DESTDIR)/usr/bin/
	cp -v download-infowars-video $(DESTDIR)/usr/bin/

