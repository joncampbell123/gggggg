
OUT=avsortui

all: $(OUT)

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp -v $(OUT) $(DESTDIR)/usr/bin/

$(OUT): avsortui.cpp
	g++ -o $@ $<

clean:
	rm -f $(OUT)


