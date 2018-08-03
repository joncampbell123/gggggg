
OUT=renameui

all: $(OUT)

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp -v $(OUT) $(DESTDIR)/usr/bin/

$(OUT): renameui.cpp
	g++ -o $@ $<

clean:
	rm -f $(OUT)


