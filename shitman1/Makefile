topdir=$(shell pwd)

SDL2_CFLAGS=$(shell pkg-config sdl2 --cflags)
SDL2_LIBS=$(shell pkg-config sdl2 --libs)

SHITMAN=$(bindir)/shitman

SHITMAN_OBJ=$(objdir)/main.o

SHITMAN_CFLAGS=$(SDL2_CFLAGS) -I$(topdir) -std=gnu99 -DLINUX=1

bindir=$(topdir)/bin-linux

objdir=$(topdir)/obj-linux

all: $(bindir) $(objdir) $(SHITMAN)

$(objdir)/main.o: main.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(bindir):
	mkdir -p $(bindir)

$(objdir):
	mkdir -p $(objdir)

$(SHITMAN): $(bindir) $(objdir) $(SHITMAN_OBJ)
	gcc -o $(SHITMAN) $(SHITMAN_OBJ) $(SDL2_LIBS)

clean:
	rm -f $(SHITMAN_OBJ)

distclean: clean
	rm -f $(SHITMAN) $(SHITMAN_OBJ)
	rm -Rf $(bindir) $(objdir)

