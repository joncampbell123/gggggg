all: pdfobjedit

pdfobjedit: pdfobjedit.cpp
	g++ -o $@ $<

clean:
	rm -f pdfobjedit

install:
	mkdir -p $(DESTDIR)/usr/bin/
	cp -v pdfobjedit $(DESTDIR)/usr/bin/

