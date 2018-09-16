#
# Makefile for lxmusserv 0.94
#

# Change these to fit your system
CC     = gcc

WFLAGS = -Wall -Wstrict-prototypes # -Wwrite-strings -W -Wcast-qual \
         -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls \
         -Wbad-function-cast -Wcast-align -Wpointer-arith -Wshadow -Wconversion

#CFLAGS = $(WFLAGS) -O2 -mpentiumpro
#CFLAGS = -Wall -g3
CFLAGS = $(WFLAGS) $(COPT.arch)
AFLAGS = $(CFLAGS)
LDFLAGS = # -s
LIBS   = 

# FreeBSD options
#CFLAGS = -I. -Wall -O2 -m486 -I. -I/usr/src/sys/i386/isa/sound
#LIBS   =

INSTALL = install -s
DESTDIR = /usr/local/games
SHELL = /bin/sh

#############################################
# Nothing below this line should be changed #
#############################################

.c.s:
	$(CC) $(AFLAGS) -S -o $*.s $<

OBJS = musserver.o readwad.o playmus.o sequencer.o midi_setup.o
.DUMMY: clean realclean archive install

all:
	@echo "Choose one of the following actions:"
	@echo
	@echo " make musserver    - will build the musicserver (from lxdoom)"
	@echo
	@echo " make clean        - will clean up object and exec files"
	@echo

musserver: dep $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) $(LIBS) -o musserver
	cp musserver ../

clean:
	rm -f $(OBJS) musserver ../musserver .depend *~

dep:
	$(CC) -E -M $(CFLAGS) *.c > .depend

archive: clean
	(cd .. ; tar cf - musserv/*.[ch] musserv/[A-Z]* \
         | gzip -9f > lxmusserv-0.94.tar.gz)

install: all
	$(INSTALL) musserv $(DESTDIR)/

ifeq (.depend,$(wildcard .depend))
include .depend
endif
