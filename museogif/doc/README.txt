Hello girls and boys,

here is the Linux port of Heretic (Current version: 1.0.3) !

You con now play Heretic with Linux/x86, Linux/m68k, Linux/Alpha,
FreeBSD, Netwinder (ARM) and SCO-Unix. (perhaps more UNIXenv - not tested)

The game supports five graphics targets under Linux.
These are X11, (fastX11), SVGAlib, GGI and SDL.

You are able to play singleplayer games, load and save and 
resize the game-screen.

The default resolution of Heretic is 320x200. You can start Heretic with a bigger
resolution and there are two ways to for it. At first there is the "-mode" switch.This switch changes the (internal) gamesresolution. If you want to start heretic
in 640x480 resolution (for example), start it with "xheretic -mode 640 480".
This feature is supported by all graphics targets.

The other way to enlarge the gamewindow is to start heretic with the "-2", "-3" or the "-4" switch. Because this simply enlarges the gamewindow after it is rendered, I recommend the first method.

You can hear music within Heretic (supports OPL3/GenMidi/AWE32-synth).

It produces realistic Ambient Sfx-sound like in the original Dos-Heretic.
(This means if you are attacked my a monster from a side, you will hear
the sound on this side !!!)

If you won't (or don't want to) hear sound, start heretic 
with the '-nosound' switch.

If you have trouble setting up music support or you won't (or don't want to)
here music, start heretic with the '-nomusic' switch.

If you don't want vgaheretic to be running even if you're switched off
the console you started it on, start vgaheretic with the '-nobgrun' switch.

Networkgames are possible for up to four players over UDP or IPX.
If you want to start a net-game(UDP) with two players, try this:

machine 1: xheretic -net 1 IP-addr-machine1 IP-addr-machine2
machine 2: xheretic -net 2 IP-addr-machine1 IP-addr-machine2

Note: The IP-addr of the machine with the flag '-net 1' should be the first
      IP-addr in the IP-list. The IP-addr's should be in the same order on
      every machine !

If you want to start a net-game(IPX) with two players, try this:

machine 1: xheretic_ipx -ipx 2
machine 2: xheretic_ipx -ipx 2

Note: You need to compile IPX-support into your Kernel ! Also you have to bind
      an IPX-interface to your netcard. It is done with the 'ipx_interface'-
      command. (The command is included with this distribution). The exact
      command is: 'ipx_interface add -p eth0 802.2 0x39ab0222'. The eth0 is
      your netcard (look with 'ifconfig') and the IPX network address is
      0x39ab0222. Mail back if you have problems with it !


There is a tcl/tk script (XHeretic-launch) in the source package now 
(needs tcl_tk 8.0). With it you can start heretic. It was created by 
Udo Munk for doom and was hacked for heretic by NoKey.


Notes for compiling the source:

>) Go to the Heretic-source-tree, (edit the Heretic Makefile, if needed) 
   and type make (and choose what you want to compile).

>) Heretic compiles and runs on Linux/m68k.
   Just edit the Makefile and add the $(COPT.m68k) to the $(CFLAGS)
   and remove the other $(COPT.xxx) !

>) To compile it on FreeBSD, edit the Makefile and add $(COPT.FreeBSD) to the
   $(CFLAGS). Don't forget to remove old(other) $(COPT.xxx).

>) Compiling Heretic on Alpha-machines shouldn't be a big problem. Just edit
   the Makefile and remove '$(COPT.x86)' at 'CFLAGS' and put a
   '$(COPT.alpha)' there.

>) If you want to compile Heretic on/with SCO-Unix, edit the Makefile and just
   remove '$(COPT.sound)' at 'CFLAGS' and compile for the standart X11-target.

>) For compiling Heretic for NetwinderPC's, look at the Makefile for details.

>) You have to choose your wanted network-protokoll. If you want UDP you don't
   need to change the Makefile; If you want IPX, only remove $(CDEFS.udp) from
   the CFLAGS and add $(CDEFS.ipx).

>) If you have a vga-lib >= version 1.3.0, edit the Makefile and comment
   -D__NEWVGALIB__ in !

>) If you have any of the extra *.lmp files, comment -DEXTRA_WADS in.

>) If you have problems compiling the source, you are free to send an
     E-Mail to  'heretic@awerthmann.de'

>) If you want to use the ggi-version of the game, look at
   http://www.ggi-project.org to get a recent version of ggi.

IMPORTANT:
-----------

>) Read 'orig_license.txt' !!!
>) The modifications to the original source code are free under the GPL.
>) Read Gamekeys.txt for the keys used in the Game.
>) You need at least the shareware-version of the Heretic WAD !
>) The GGI-version runs in 8, 15, 16 and 32 bpp. If you want to run it on 
   a 24bpp X display you can use the palemu target 
   (GGI_DISPLAY=palemu ggiheretic). Note that in higher depths GGI-on-X is 
   much faster than the native X11-version, especially on a scaled display. 
>) The X11-version runs under every colordepth above 8bpp, but performs best 
   under 8bpp mode 
>) The SVGAlib-version runs under a console. You must be root to run
   vgaheretic, or set the suid-bit for vgaheretic.
   You can run vgaheretic in background while you have switched to
   an other virtual-console.
>) If you have the shareware WAD, you must rename (or symlink) it to
   heretic_share.wad   !
>) If you have the full-commercial WAD, you must rename (or symlink) it to
   heretic.wad   !
>) There should be the 'sndserver' file in the Heretic-directory, look for it.
   If not, go to the sndserv directory, type make and go back.
>) Savegames and the config-file 'heretic.cfg' are kept in the directory
   ~/.heretic ! The WAD is searched in the actual directory, if not found it is
   searched in the dir pointed by the env 'HERETICHOME' and if not found there,
   it is searched in the directory ~/.heretic then.
>) If you don't want to use OPL3 and you have a GeneralMidi or a AWE32 (64),
   edit the generated ~/.heretic/heretic.cfg and change musopts "" to 
   musopts "-m" for GenMidi or musopts "-a" for AWE 32 .
   (For detailed commands of the musserver execute './musserver -h')
>) If you want linear or bilinear filtering (renices the gamescreen) start
   heretic with '-linear' or '-bilinear' parameter.
>) The fast X11 target only works on an Intel PC yet.
>) The program 'ipx_interface' was done by Greg Page <greg@caldera.com>.


TODO:
-----

>) New Feautures !!!
