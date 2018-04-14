topdir=$(shell pwd)

SDL2_CFLAGS=$(shell pkg-config sdl2 --cflags)
SDL2_LIBS=$(shell pkg-config sdl2 --libs)

SHITMAN=$(bindir)/shitman

SHITMAN_OBJ=$(objdir)/main.o $(objdir)/gif_lib.o $(objdir)/game_kb.o $(objdir)/game_ks.o $(objdir)/game_tm.o $(objdir)/game_lgf.o $(objdir)/game_kev.o $(objdir)/game_pal.o

SHITMAN_CFLAGS=$(SDL2_CFLAGS) -I$(topdir) -std=gnu99 -DLINUX=1

bindir=$(topdir)/bin-linux

objdir=$(topdir)/obj-linux

resdir=$(topdir)/res

all: $(bindir) $(objdir) $(SHITMAN) shitmanres

run: $(SHITMAN) shitmanres
	cd $(bindir) && ./shitman

run-gdb: $(SHITMAN) shitmanres
	cd $(bindir) && gdb --args ./shitman

run-vg: $(SHITMAN) shitmanres
	cd $(bindir) && valgrind --leak-check=full --show-reachable=yes --track-origins=yes ./shitman

shitmanres: $(bindir)
	cp -f $(resdir)/* $(bindir)/

$(objdir)/main.o: main.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/game_kb.o: game_kb.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/game_ks.o: game_ks.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/game_tm.o: game_tm.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/game_kev.o: game_kev.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/game_lgf.o: game_lgf.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/game_pal.o: game_pal.c
	gcc -c -o $@ $< $(SDL2_CFLAGS) $(SHITMAN_CFLAGS)

$(objdir)/gif_lib.o: gif_lib.c
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

