/*************************************************************************
 *  midi_setup.c
 *
 *  Many changes and cleanups for l[xs]doom (see NOTES file)
 *
 *  Copyright (c) 1999 Rafael Reilova (rreilova@ececs.uc.edu)
 *
 *  Copyright (c) 1997 Takashi Iwai (iwai@dragon.mm.t.u-tokyo.ac.jp)
 *
 *  Original Source
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

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "musserver.h"
#include "sequencer.h"

int seqfd = -1;
int seq_dev = -1;
synthdev_t synth_type;

struct synth_info fm_sinfo;

#ifdef ENABLE_AWE
static void detect_awe(int numsynths)
{
  int i;
  int start = 0;
  int end = numsynths - 1;
  struct synth_info sinfo;

  if (seq_dev >= 0)
    {
      start = end = seq_dev;
      seq_dev = -1;
    }

  for (i = start; i <= end; i++)
    {
      sinfo.device = i;
      if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &sinfo) == -1)
	{
	  seq_dev = -1;
	  cleanup(IO_ERROR, "detect_awe: ioctl(SNDCTL_SYNTH_INFO)");
	}
      if (sinfo.synth_type == SYNTH_TYPE_SAMPLE &&
	  sinfo.synth_subtype == SAMPLE_TYPE_AWE32)
	{
	  seq_dev = i;
	  if (verbose)
	      printf("device %d: %s\n", i, sinfo.name);
	}
    }
}
#endif

static void detect_midi(int nummidis)
{
  struct midi_info minfo;
  int i;
  int start = 0, end = nummidis - 1;

  if (seq_dev >= 0)
    {
      start = end = seq_dev;
      seq_dev = -1;
    }

  for (i = start; i <= end; i++)
    {
      minfo.device = i;
      if (ioctl(seqfd, SNDCTL_MIDI_INFO, &minfo) == -1)
	{
	  seq_dev = -1;
	  cleanup(IO_ERROR, "detect_midi: ioctl(SNDCTL_MIDI_INFO)");
	}
      if (verbose)
	  printf("device %d: %s\n", i, minfo.name);
    }
  seq_dev = i - 1;
}

static void detect_fm(int numsynths)
{
  int i;
  int start = 0;
  int end = numsynths - 1;
  int opl3_cap = 0;
  struct synth_info sinfo;

  if (seq_dev >= 0)
    {
      start = end = seq_dev;
      seq_dev = -1;
    }

  for (i = start; i <= end; i++)
    {
      sinfo.device = i;
      if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &sinfo) == -1)
	{
	  seq_dev = -1;
	  cleanup(IO_ERROR, "detect_fm: ioctl(SNDCTL_SYNTH_INFO)");
	}
      if (sinfo.synth_type == SYNTH_TYPE_FM)
	{
	  opl3_cap = sinfo.capabilities & SYNTH_CAP_OPL3;
	  seq_dev = i;
	  fm_sinfo = sinfo;
	  if (verbose)
	    printf("device %d: %s (%d voices%s)\n",
		   i, fm_sinfo.name, fm_sinfo.nr_voices,
		   opl3_cap ? " OPL3" : "");
	}
    }

  if (seq_dev == -1)
    return;

  if (has_opl3 == -1) /* auto-select */
    has_opl3 = opl3_cap;

  fmvoices = (struct fm_voice *) malloc(sizeof(struct fm_voice)
					*fm_sinfo.nr_voices);
  if (!fmvoices)
    {
      seq_dev = -1;
      cleanup(MISC_ERROR, "fm_setup: can't allocate memory");
    }

  /* initialize voices state */
  for (i = 0; i < fm_sinfo.nr_voices; i++)
    {
      fmvoices[i].active  =  0;
      fmvoices[i].note    = -1;
      fmvoices[i].channel = -1;
      fmvoices[i].patch   = -1;
      fmvoices[i].time    = -1;
    }

  mix_fd = open("/dev/mixer", O_WRONLY);
  if (mix_fd != -1)
  {
    int dummy;

    /* test that mixer device and synth-channel actually exist */
    if (ioctl(mix_fd, SOUND_MIXER_READ_SYNTH, &dummy) == -1)
      {
	close(mix_fd);
	mix_fd = -1;
      }
    else if (verbose)
      printf("%s: will use /dev/mixer to control FM-volume\n", progname);
  }
}

int midi_setup(synthdev_t pref_dev, int num_dev)
{
  int numsynths;
  int nummidis;

  if ((seqfd = open("/dev/sequencer", O_WRONLY, 0)) == -1) {
    warning(IO_ERROR, "midi_setup: error opening /dev/sequencer");
    return NO_PATCH_NEEDED;
  }

  if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &numsynths) == -1)
    cleanup(IO_ERROR, "midi_setup: ioctl(SNDCTL_SEQ_NRSYNTHS)");

  if (ioctl(seqfd, SNDCTL_SEQ_NRMIDIS, &nummidis) == -1)
    cleanup(IO_ERROR, "midi_setup: ioctl(SNDCTL_SEQ_NRMIDIS)");

  timer_res = 0;
  if (ioctl(seqfd, SNDCTL_SEQ_CTRLRATE, &timer_res) == -1)
    cleanup(IO_ERROR, "midi_setup: ioctl(SNDCTL_SEQ_CTRLRATE)");

  seq_dev = num_dev;
  switch (pref_dev)
    {
    case LIST_ONLY_SYNTH:
      seq_dev = -1;
      if (numsynths)
	{
	  verbose++;
	  puts("\nAvailable AWE devices (use -a):");
#ifdef ENABLE_AWE
	  detect_awe(numsynths);
	  if (seq_dev == -1)
	    puts("None");
	  seq_dev = -1;
#else
	  puts("AWE support not compiled in");
#endif
	  puts("\nAvailable FM devices (use -f):");
	  detect_fm(numsynths);
	  if (seq_dev == -1)
	    puts("None");
	  seq_dev = -1;
	}
      if (nummidis)
	{
	  puts("\nAvailable MIDI devices (use -m):");
	  detect_midi(numsynths);
	  if (seq_dev == -1)
	    puts("None");
	}
      seq_dev = -1;
      cleanup(CLEAN_EXIT, NULL);

    case USE_BEST_SYNTH:
      seq_dev = -1;

#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      if (numsynths)
	{
	  detect_awe(numsynths);
	  if (seq_dev >= 0) /* AWE found */
	    {
	      synth_type = USE_AWE_SYNTH;
	      return NO_PATCH_NEEDED;
	    }
	  if (pref_dev == USE_AWE_SYNTH)
	    cleanup(IO_ERROR, "midi_setup: no AWE synth device found");
	}
      /* fall-thru */
#endif

      /*
       * I know FM synth isn't better than general midi, but the idea
       * is that this should work the first time, those that have
       * real midi can override this, OTOH, most people don't have
       * midi devices attached to their sound cards, but do have a
       * MIDI UART in-there that would get detected otherwise!
       */
    case USE_FM_SYNTH:
      if (numsynths)
	{
	  detect_fm(numsynths);
	  if (seq_dev >= 0) /* FM synth found */
	    {
	      synth_type = USE_FM_SYNTH;
	      return PATCH_NEEDED;
	    }
	  if (pref_dev == USE_FM_SYNTH)
	    cleanup(MISC_ERROR, "midi_setup: no FM synth device found");
	}
      /* fall-thru */

    case USE_MIDI_SYNTH:
      if (nummidis)
	{
	  detect_midi(nummidis);
	  if (seq_dev >= 0) /* MIDI device found */
	    {
	      synth_type = USE_MIDI_SYNTH;
	      return NO_PATCH_NEEDED;
	    }
	  if (pref_dev == USE_MIDI_SYNTH)
	    cleanup(MISC_ERROR, "midi_setup: no MIDI synth device found");
	}
      /* fall-thru */

    default:
      cleanup(CLEAN_EXIT, "midi_setup: no synth device found");
      break;
    }
}
