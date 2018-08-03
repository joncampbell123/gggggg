
OUT=renameui

all: $(OUT)

install:
	mkdir -p $(DESTDIR)/usr
	cp -v $(OUT) $(DESTDIR)/usr/

$(OUT): renameui.cpp
	g++ -o $@ $<

clean:
	rm -f $(OUT)


