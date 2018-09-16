CC = gcc

CFLAGS=	-Wall -Wstrict-prototypes $(COPT.arch)

LIBS=	-lm

OBJS=	soundsrv.o sounds.o wadread.o


all: sndserver.oss sndserver.sdl

sndserver.oss:	dep $(OBJS) linux.o
		$(CC) $(CFLAGS) $(OBJS) linux.o $(LIBS) -o sndserver.oss
		cp sndserver.oss ../

sndserver.sdl:	dep $(OBJS) linux_sdl.o
		$(CC) $(CFLAGS) $(OBJS) linux_sdl.o -lSDL -pthread $(LIBS) -o sndserver.sdl
		cp sndserver.sdl ../

sndserver: sndserver.sdl sndserver.oss
		ln -sf sndserver.sdl ../sndserver

clean:
		rm -f *.o sndserver.oss ../sndserver.oss sndserver.sdl ../sndserver.sdl sndserver ../sndserver .depend *~

dep:
		$(CC) -E -M $(CFLAGS) *.c > .depend

%.o: 		%.c	
		$(CC) $(CFLAGS) -c $< -o $@

ifeq (.depend,$(wildcard .depend))
include .depend
endif
