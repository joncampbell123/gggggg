# Makefile for Linux/BSD-Heretic

CC = gcc

#
# For old SVGALIB, comment out -D__NEWVGALIB__
#

CDEFS.ipx = -DUNIX -DHAVE_USLEEP -DHAVE_MATH_H -DHAVE_VALUES_H -DLINUX_MOUSE \
	-DIPX_PROTOCOL -DI_GGI_HERETIC -DNEED_SHMGETEVENTBASE \
	-D__NEWVGALIB__ # -DIPX_DEBUG -DEXTRA_WADS

CDEFS.udp = -DUNIX -DHAVE_USLEEP -DHAVE_MATH_H -DHAVE_VALUES_H -DLINUX_MOUSE \
	-DUDP_PROTOCOL -DI_GGI_HERETIC -DNEED_SHMGETEVENTBASE \
	-D__NEWVGALIB__ # -DEXTRA_WADS


# Uncomment the line below if you want to use GSI for sound.
# (doesn't really work yet. :-()
#WANT_GSI = yes


# Plattform specific definitions
# _begin_
COPT.x86 = -march=pentium -O2 -falign-jumps=2 -falign-functions=2 -D__32BIT__ \
		-DHAVE_ALLOCA_H

COPT.x86_64 = -march=athlon64 -O2 -falign-jumps=4 -falign-functions=4 -D__64BIT__ \
		-DHAVE_ALLOCA_H

COPT.m68k = -m68020-60 -O2 -fomit-frame-pointer -D__BIG_ENDIAN__ -D__32BIT__ \
		-DHAVE_ALLOCA_H

COPT.alpha = -O2 -D__64BIT__ -DHAVE_ALLOCA_H

COPT.FreeBSD = -m486 -O2 -malign-jumps=2 -malign-functions=2 -D__32BIT__

COPT.arm = -O2 -D__32BIT__ -fsigned-char -fomit-frame-pointer \
           -DHAVE_ALLOCA_H -D__PACKED__=__attribute__\ \(\(packed\)\)
# _end_

plat=$(shell uname -m)

# Change this to change the platform
ifeq ($(plat),x86_64)
COPT.arch = $(COPT.x86_64)
else
COPT.arch = $(COPT.x86)
endif

# If you want IPX-netcode compiled, replace $(CDEFS.udp) with $(CDEFS.ipx)

CFLAGS = -Wall -Wstrict-prototypes -Wextra -Wno-strict-aliasing $(CDEFS.udp) \
	 -I. -I.. -I/usr/X11R6/include $(COPT.arch) $(COPT.sound) -g3 -O0

# Uncomment the line below if you want to use original "[" and "]" keys
# for inventory access, instead of "k" and "l".
#CFLAGS += -DORIG_INVKEYS

ifeq ($(WANT_GSI),yes)
SOUND_OBJS = gsisound/i_sound.o gsisound/soundst.o gsisound/sounds.o \
	     gsisound/gsi.o m_misc.o
SOUND_REFS = gsisound/i_sound.c gsisound/soundst.c gsisound/sounds.c \
	     gsisound/gsi.c m_misc.c

COPT.sound = -D__DOSOUND__ -D__DOMUSIC__ -DUSE_GSI -Igsisound
X11LIBS = -Xext -lX11 -lm
else
SOUND_OBJS = soundclient/i_sound.o soundclient/soundst.o soundclient/sounds.o \
	     m_misc.o
SOUND_REFS = soundclient/i_sound.c soundclient/soundst.c soundclient/sounds.c \
	     m_misc.c

COPT.sound = -D__DOSOUND__ -DSNDSERV -Isoundclient -D__DOMUSIC__ -DMUSSERV # -D_DEBUGSOUND
X11LIBS = -lXext -lX11

MUSSERV = musserver
SNDSERV = sndserver

endif

X_LDFLAGS = -L/usr/X11R6/lib

X11LIBS = -lXext -lX11
GGILIBS = -lggi -lm
VGALIBS = -lvga
SDLLIBS = -lSDL -lpthread 

OBJS =	am_map.o ct_chat.o d_main.o d_net.o f_finale.o g_game.o \
	p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o \
	p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_setup.o p_sight.o \
	p_spec.o p_switch.o p_telept.o  p_tick.o p_user.o r_bsp.o r_data.o \
	r_draw.o r_plane.o r_segs.o r_things.o r_main.o mn_menu.o sb_bar.o \
	tables.o v_video.o w_wad.o z_zone.o in_lude.o \
	info.o i_net.o i_system.o i_udp.o i_ipx.o i_main.o $(SOUND_OBJS)

all:	
	@echo "Choose one of following actions:"
	@echo
	@echo " make x11       - will build xheretic - runs under X11"
	@echo
	@echo " make fastx11   - will build xaheretic (fast X-version) - runs under X11"
	@echo
	@echo " make ggi       - will build ggiheretic - runs under GGI"
	@echo		
	@echo " make vga       - will build vgaheretic - runs under SVGAlib"
	@echo
	@echo " make sdl       - will build sdlheretic - runs under SDL-lib"
	@echo
	@echo " make sndserver - will build the soundserver for Heretic"
	@echo
	@echo " make musserver - will build the musicserver (from lxdoom) for Heretic"
	@echo
	@echo " make clean     - will clean up object and exec files"
	@echo

all-targets: x11 fastx11 sdl sndserver musserver

clean:	
	rm -f $(OBJS) xheretic xaheretic ggiheretic vgaheretic \
	sdlheretic graphics/i_x11.o graphics/i_x11_fast.o \
	graphics/i_ggi.o graphics/i_vga.o graphics/i_sdl.o
	(cd sndserv; $(MAKE) clean; cd ..) || exit 1
	(cd musserv; $(MAKE) clean; cd ..) || exit 1

distclean:
	rm -f $(OBJS) xheretic xaheretic ggiheretic vgaheretic \
        sdlheretic graphics/i_x11.o graphics/i_x11_fast.o \
	graphics/i_sdl.o graphics/*.orig \
        graphics/i_ggi.o graphics/i_vga.o graphics/*~ graphics/*.rej \
        .depend soundclient/*~ gsisound/*~ sndserv/*~ *~ *.rej *.orig \
	doc/*~ doc/*.rej doc/*.orig
	(cd sndserv; $(MAKE) clean; cd ..) || exit 1
	(cd musserv; $(MAKE) clean; cd ..) || exit 1

depx11:
	$(CC) -E -M $(CFLAGS) $(X_LDFLAGS) *.c $(SOUND_REFS) \
	graphics/i_x11.c > .depend

depx11f:
	$(CC) -E -M $(CFLAGS) $(X_LDFLAGS) *.c $(SOUND_REFS) \
	graphics/i_x11.c > .depend

depggi:
	$(CC) -E -M $(CFLAGS) $(X_LDFLAGS) *.c $(SOUND_REFS) \
	graphics/i_ggi.c > .depend

depvga:
	$(CC) -E -M $(CFLAGS) $(X_LDFLAGS) *.c $(SOUND_REFS) \
	graphics/i_vga.c > .depend

depsdl:
	$(CC) -E -M $(CFLAGS) $(X_LDFLAGS) *.c $(SOUND_REFS) \
	graphics/i_sdl.c > .depend                                                               

x11:	depx11 $(OBJS) graphics/i_x11.o $(MUSSERV) $(SNDSERV)
	$(CC) $(CFLAGS) $(X_LDFLAGS) $(X11LIBS) $(OBJS) \
	graphics/i_x11.o -o xheretic

fastx11:depx11f $(OBJS) graphics/i_x11_fast.o $(MUSSERV) $(SNDSERV)
	$(CC) $(CFLAGS) $(X_LDFLAGS) $(X11LIBS) $(OBJS) \
	graphics/i_x11_fast.o -o xaheretic

ggi:	depggi $(OBJS) graphics/i_ggi.o $(MUSSERV) $(SNDSERV)
	$(CC) $(CFLAGS) $(GGILIBS) $(OBJS) \
	graphics/i_ggi.o -o ggiheretic

vga:	depvga $(OBJS) graphics/i_vga.o $(MUSSERV) $(SNDSERV)
	$(CC) $(CFLAGS) $(VGALIBS) $(OBJS) \
	graphics/i_vga.o -o vgaheretic

sdl:    depsdl $(OBJS) graphics/i_sdl.o $(MUSSERV) $(SNDSERV)
	$(CC) $(CFLAGS) $(X_LDFLAGS) $(SDLLIBS) $(OBJS) \
	graphics/i_sdl.o -o sdlheretic

sndserver:
	$(MAKE) -C sndserv COPT.arch="$(COPT.arch)" $@

musserver:
	$(MAKE) -C musserv COPT.arch="$(COPT.arch)" $@

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@


ifeq (.depend,$(wildcard .depend))
include .depend
endif
