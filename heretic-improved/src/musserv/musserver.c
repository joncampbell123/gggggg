/*************************************************************************
 *  musserver.c
 *
 *  update to lxdoom, assorted features and bugfixes (see NOTES)
 *  Copyright (C) 1999 Rafael Reilova (rreilova@ececs.uc.edu)
 *
 *  Copyright (C) 1995 Michael Heasley (mheasley@hmc.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "musserver.h"
#include "sequencer.h"

/* define for manual mode MUS playing - debugging use only */
#undef TESTING_CMDS

char *progname;
int verbose;
int has_opl3 = -1;
int dumpmus;

static synthdev_t pref_dev = USE_BEST_SYNTH;
static int num_dev = -1;

static FILE *musfile;
static FILE *oplfile;

/*
 * try to leave the sequencer in a sane state before exiting
 */
static void sig_hdlr(int snum __attribute__ ((unused)))
{
  cleanup(CLEAN_EXIT, "signal caught, exiting");
}

static void show_help(void)
{
  puts("Usage: musserver [options] [MUSfile [FM-patch file]]\n\n"
#ifdef ENABLE_AWE
       "  -a\t\tuse AWE32 synth device for music playback\n"
#endif
       "  -f\t\tuse FM synth device for music playback\n"
       "  -h\t\tprint this message and exit\n"
       "  -l\t\tlist detected music devices and exit\n"
       "  -m\t\tuse general midi device for music playback\n"
       "  -v\t\tverbose\n"
       "  -u number\tuse device [number] as reported by 'musserver -l'\n"
       "  -2\t\tforce dual voice mode when using FM synthesis\n"
       "  -1\t\tforce single voice mode when using FM synthesis\n"
       "  -B\t\tdo not increase pitch by one octave when using FM synthesis\n"
       "  -D\t\tdump playing songs and FM-patches to disk as mus-files\n"
       "\t\tsongX.mus and raw_midi.patch respectively.\n");
}

void warning(int error, const char *mesg)
{
  if (mesg)
    printf("%s: %s\n", progname, mesg);

  if (error == IO_ERROR && errno)
    perror(progname);
}

void cleanup(int error, const char *mesg)
{
  if (seq_dev != -1)
    cleanup_midi();

  if (mesg)
    printf("%s: %s\n", progname, mesg);

  if (error == IO_ERROR && errno)
    perror(progname);

  exit(error);
}

static void parse_arguments(int argc, char *argv[])
{
  static const char opt_conflict[] =
    "please specify only one of the -a -f -m -l options";
  int x;

  progname = argv[0];

  musfile = stdin;
  oplfile = stdin;

  while ((x = getopt(argc, argv, "afhlmu:v12BD")) != -1)
    switch (x)
      {
      case 'a':
	if (pref_dev == USE_BEST_SYNTH) {
#ifdef ENABLE_AWE
	  pref_dev = USE_AWE_SYNTH;
#else
	  cleanup(MISC_ERROR,
	    "This program version wasn't compiled with AWE driver support\n");
#endif
	}
        else
          cleanup(MISC_ERROR, opt_conflict);
        break;

      case 'f':
        if (pref_dev == USE_BEST_SYNTH)
	  pref_dev = USE_FM_SYNTH;
        else
          cleanup(MISC_ERROR, opt_conflict);
        break;

      case 'm':
        if (pref_dev == USE_BEST_SYNTH)
	  pref_dev = USE_MIDI_SYNTH;
        else
          cleanup(MISC_ERROR, opt_conflict);
        break;

      case 'h':
        show_help();
        exit(0);
        break;

      case 'l':
        if (pref_dev == USE_BEST_SYNTH)
	  {
	    pref_dev = LIST_ONLY_SYNTH;
	    verbose = 1;
	  }
        else
          cleanup(MISC_ERROR, opt_conflict);
        break;

      case 'u':
	if (pref_dev == USE_BEST_SYNTH || pref_dev == LIST_ONLY_SYNTH)
	  cleanup(MISC_ERROR,
		  "specify device type, -a -f -m, to use the -u option");

        num_dev = atoi(optarg);
        break;

      case 'v':
        verbose++;
        break;
      case '?': case ':':
        show_help();
        cleanup(MISC_ERROR, NULL);
        break;

      case '1':  /* force use of only a single voice per instrument */
	has_opl3 = 0;
	break;

	/*
	 * force use of dual voices instruments even on cards with
	 * limited set of voices (Adlib), not recommended
	 */
      case '2':
	has_opl3 = 1;
	break;

      case 'B': /* don't increase all notes by one octave */
	voxware_octave_bug = 0;
	break;

      case 'D':
	dumpmus = 1;
	break;
      }

  if (argv[optind])
    {
      musfile = fopen(argv[optind], "r");
      if (!musfile)
	cleanup(IO_ERROR, "Can't open MUS file");

      if (argv[++optind])
	{
	  oplfile = fopen(argv[optind], "r");
	  if (!oplfile)
	    cleanup(IO_ERROR, "Can't open OPL instrument file");
	}
      else
	oplfile = NULL;

      if (dumpmus)
	{
	  printf("%s: -D non-sensical when standalone, ignoring",progname);
	  dumpmus = 0;
	}
    }
}


int main(int argc, char **argv)
{
  const MUSFILE *song = NULL;
  const OPLFILE *midifile;
  int vol;
  int load_midi;
  int looping = 0;
  int playing_song = 0;
  int paused = 0;
  int ch;

  parse_arguments(argc, argv);

  load_midi = midi_setup(pref_dev, num_dev);

  if (!oplfile && load_midi)
    cleanup(MISC_ERROR, "FM synthesis requires an instrument patch file");

  if (oplfile == stdin || load_midi)
    {
      midifile = read_genmidi(oplfile);
      if (verbose)
	printf("%s: got MIDI instrument info\n", progname);

      if (load_midi)
	{
	  fmload(midifile->opl_instruments);
	  if (verbose)
	    printf("%s: Loaded MIDI into synth device\n", progname);
	}
    }

  signal(SIGINT, sig_hdlr);
  signal(SIGTERM, sig_hdlr);

#ifndef TESTING_CMDS
  if (musfile != stdin)
    {
      song = readmus(musfile);
      vol_change(DFL_VOL);
      midi_timer(MIDI_START);
      playmus(song, MUS_NEWSONG, MUS_NOLOOP, STDIN_FILENO);
      tcflush(STDIN_FILENO, TCIFLUSH);
      cleanup(CLEAN_EXIT, NULL);
    }
#endif

  /* main loop */
  while((ch = fgetc(stdin)) != EOF)
    {
      switch (ch)
	{
	  /* volume change */
	case 'V':
	  if (fscanf(stdin, "%d%*c", &vol) != 1)
	      cleanup(IO_ERROR, "error reading new volume");

	  vol_change(vol);
	  if (verbose > 1)
	    printf("%s: volume change to %d\n", progname, vol);
	  if (playing_song && !paused)
	    playmus(song, MUS_SAMESONG, looping, STDIN_FILENO);
	  break;

	  /* stop song */
	case 'S':
	  if (playing_song)
	    {
	      midi_timer(MIDI_STOP);
	      playing_song = 0;
	      if (verbose > 1)
		printf("%s: song stopped\n", progname);
	    }
	  break;

	  /* pause song */
	case 'P':
	  if (playing_song && !paused)
	    {
	      midi_timer(MIDI_PAUSE);
	      paused = 1;
	      if (verbose > 1)
		printf("%s: song paused\n", progname);
	    }
	  break;

	  /* resume song */
	case 'R':
	  if (playing_song)
	    {
	      if (paused)
		{
		  paused = 0;
		  midi_timer(MIDI_RESUME);
		  if (verbose > 1)
		    printf("%s: song resumed\n", progname);
		}
	      playmus(song, MUS_SAMESONG, looping, STDIN_FILENO);
	    }
	  break;

	  /* new song */
	case 'N':
	  if (fscanf(stdin, "%d%*c", &looping) != 1)
	    cleanup(IO_ERROR, "error reading \"looping\" flag");

	  song = readmus(musfile);
	  if (verbose > 1)
	    printf("%s: read new song, starting music\n", progname);

	  playing_song = 1;
	  paused = 0;
	  midi_timer(MIDI_START);
	  playmus(song, MUS_NEWSONG, looping, STDIN_FILENO);
	  break;

	  /* Quit command */
	case 'Q':
	  cleanup(CLEAN_EXIT, (verbose) ? "received the quit command" : NULL);

	case '\n':  /* Ignore newline delimiters */
	  continue;

	default:
	  cleanup(MISC_ERROR, "read unrecognized command");
	}
    }
  cleanup(MISC_ERROR, "error reading mesg. pipe");
}
