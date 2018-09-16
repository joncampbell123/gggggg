/*************************************************************************
 *  sequencer.c
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "musserver.h"
#include "sequencer.h"


/* define  for copious debugging output (full sequencer trace) */
#undef SEQ_DEBUG

/* define for trace output from FM-specific sequencer routines */
#undef FM_DEBUG

/* define to install a SIGWINCH handler that will dump FM-state info */
#undef INSTALL_SIGH

#ifdef INSTALL_SIGH
#include <signal.h>
static int droppednotes;
#endif

/*
 * sequencer volume state
 */
static int chanvol[N_CHN];
static int volscale;

/*
 * FM-state synth variables
 */
struct fm_voice *fmvoices;
static int voice2_map[N_INTR];
static int fmpatches[N_CHN];
static int fm_pan[N_CHN], fm_bender[N_CHN];
static const struct opl_instr *fm_instruments;
static int free_voices;

/* debug statistics and misc */
int voxware_octave_bug = 1;
int mix_fd = -1;

SEQ_DEFINEBUF(2048);

void seqbuf_dump(void)
{
  if (_seqbufptr && seqfd != -1)
    if (write(seqfd, _seqbuf, _seqbufptr) == -1)
      {
	close(seqfd);
	seq_dev = -1;
	cleanup(IO_ERROR, "write: /dev/sequencer");
      }
  _seqbufptr = 0;
}

/* convert Doom's OPL format to SBI */
static void map_opl2sbi(const struct OPL2instrument *fmi,
			unsigned char *operators)
{
  operators[0]  = fmi->trem_vibr_1;
  operators[1]  = fmi->trem_vibr_2;
  operators[2]  = fmi->scale_1 | fmi->level_1;
  operators[3]  = fmi->scale_2 | fmi->level_2;
  operators[4]  = fmi->att_dec_1;
  operators[5]  = fmi->att_dec_2;
  operators[6]  = fmi->sust_rel_1;
  operators[7]  = fmi->sust_rel_2;
  operators[8]  = fmi->wave_1;
  operators[9]  = fmi->wave_2;
  operators[10] = fmi->feedback;
}


void fmload(const struct opl_instr *fm_instr)
{
  struct sbi_instrument sbi_fm;
  int i;
  int second_voice = N_INTR;

  fm_instruments = fm_instr;

#ifdef INSTALL_SIGH
  signal(SIGWINCH, sigwinch_hdlr);
#endif

  for (i = 0; i < N_INTR; i++)
    {
      sbi_fm.key = FM_PATCH;
      sbi_fm.device = seq_dev;
      sbi_fm.channel = i;
      map_opl2sbi(&fm_instruments[i].patchdata[0],
		  (unsigned char *) &sbi_fm.operators);
      SEQ_WRPATCH(&sbi_fm, sizeof(struct sbi_instrument));

      /*
       * implement the fake 4-OP synthesis only if we have an OPL3
       * so we don't run out of voices so easily, there's no real
       * h/w requirement, but an OPL2 only has 9 voices and will
       * likely run out of them real quickly with this enabled. 
       */
      if ((fm_instruments[i].flags & FL_DOUBLE_VOICE) && has_opl3)
	{
	  sbi_fm.key = FM_PATCH;
	  sbi_fm.device = seq_dev;
	  voice2_map[i] = sbi_fm.channel = second_voice++;
	  map_opl2sbi(&fm_instruments[i].patchdata[1],
		      (unsigned char *) &sbi_fm.operators);
	  SEQ_WRPATCH(&sbi_fm, sizeof(struct sbi_instrument));
	}
    }
}

/*
 * Turn all notes off and set all channel controllers to default values,
 * also preset current channel volume.  Called before playing each song
 */
static void reset_midi(void)
{
  int channel;
  int defvol = MAIN_VOL_NRML;

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      AWE_NOTEOFF_ALL(seq_dev);
      AWE_SET_CHANNEL_MODE(seq_dev, AWE_PLAY_MULTI);
      AWE_DRUM_CHANNELS(seq_dev, 1<<CHN_PERCUSSION);
      defvol = defvol*volscale/100;

      for (channel = 0; channel < N_CHN; channel++)
	{
	  SEQ_CONTROL(seq_dev, channel, CTRL_PITCH_BENDER_RANGE, 200);
	  SEQ_BENDER(seq_dev, channel, PITCH_CENTER);
	  SEQ_CONTROL(seq_dev, channel, CTL_PAN, PAN_CENTER);
	  chanvol[channel] = defvol;
	}
      break;
#endif
    case USE_MIDI_SYNTH:
      defvol = defvol*volscale/100;
      for (channel = 0; channel < N_CHN; channel++)
	{
	  /* all notes off */
	  SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
	  SEQ_MIDIOUT(seq_dev, 0x7b);
	  SEQ_MIDIOUT(seq_dev, 0);
	  /* reset all controllers */
	  SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
	  SEQ_MIDIOUT(seq_dev, 0x79);
	  SEQ_MIDIOUT(seq_dev, 0);
	  chanvol[channel] = defvol;
	}
      break;

    case USE_FM_SYNTH:
      for (channel = 0; channel < fm_sinfo.nr_voices; channel++)
	{
	  if (fmvoices[channel].active)
	    SEQ_STOP_NOTE(seq_dev, channel, fmvoices[channel].note, 64);

	  fmvoices[channel].active  =  0;
	  fmvoices[channel].time    = -1;
	  SEQ_CONTROL(seq_dev, channel, CTRL_PITCH_BENDER_RANGE, 200);
	  SEQ_BENDER(seq_dev, channel, PITCH_CENTER);
	  SEQ_CONTROL(seq_dev, channel, CTL_PAN, PAN_CENTER);
	  /* FM volume isn't scaled here since we use the mixer for that */
	  SEQ_CONTROL(seq_dev, channel, CTL_MAIN_VOLUME, defvol<<FMVOL_ADJ);
	}

      for (channel = 0; channel < N_CHN; channel++)
	{
	  fm_bender[channel] = PITCH_CENTER;
	  fm_pan[channel]    = PAN_CENTER;
	  chanvol[channel]   = defvol;
	}
      free_voices = fm_sinfo.nr_voices;
    default:
      break;
    }
  SEQ_DUMPBUF();
}


void cleanup_midi(void)
{
  reset_midi();
  if (seqfd != -1) close(seqfd);
  if (mix_fd != -1)
    close(mix_fd);
}


void note_off(int note, int channel, int velocity)
{
#ifdef SEQ_DEBUG
  printf("note_off(note=%d, channel=%d, velocity=%d)\n", note, channel, velocity);
#endif

  switch(synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      SEQ_STOP_NOTE(seq_dev, channel, note, velocity);
      break;
#endif
    case USE_MIDI_SYNTH:
      SEQ_MIDIOUT(seq_dev, MIDI_NOTEOFF | channel);
      SEQ_MIDIOUT(seq_dev, note);
      SEQ_MIDIOUT(seq_dev, velocity);
      break;

    case USE_FM_SYNTH:
      {
	int i;

	for (i = 0; i < fm_sinfo.nr_voices; i++)
	  if ((fmvoices[i].note == note) && (fmvoices[i].channel == channel))
	    {
#ifdef FM_DEBUG
	      printf("fm_note_off: killing voice %d\n", i);
#endif
	      fmvoices[i].active = 0;
	      fmvoices[i].time   = curtime;
	      SEQ_STOP_NOTE(seq_dev, i, note, velocity);
	      free_voices++;
	    }
      }
    default:
      break;
    }
}


#define VALID_PERC(note) (note < 82 && note >= 35)

  /*
   * some instruments have loong release values, to avoid clicks
   * and pops when reprogramming voices, get the one that has been
   * silent for the longest time, unless it is the same note & channel.
   * Important: try to stick instruments to the same voices
   */
static inline int fm_pick_from_free(int note, int secondary, int channel)
{
  int voice;
  int old_time, old_voice;
  int perc_chn;
  int valid_perc = 0;  /* see addditional comments on fm_start_voce() */
  int same_chn = 0;

  perc_chn = (channel == CHN_PERCUSSION);
  if (perc_chn)
    valid_perc = VALID_PERC(note);

  old_voice = 0;
  old_time = curtime;
  for (voice = 0; voice < fm_sinfo.nr_voices; voice++)
    {
      if (!fmvoices[voice].active)
	{
	  /* important first choice - same channel, same patch */
	  if (fmvoices[voice].channel == channel &&
	      fmvoices[voice].secondary == secondary)
	    {
	      if (fmvoices[voice].note == note)
		  return voice; /* same note! this is it! */

	      /* at least same channel */
	      if (!perc_chn || (!valid_perc &&
				!VALID_PERC(fmvoices[voice].note)))
		{
		  if (!same_chn)
		    {
		      same_chn = 1;
		      old_time = fmvoices[voice].time;
		      old_voice = voice;
		    }
		  else if (fmvoices[voice].time < old_time)
		    {
		      old_time = fmvoices[voice].time;
		      old_voice = voice;
		    }
		  continue;
		}
	    }

	  /* otherwise find an voice that has been silent for a while */
	  if (!same_chn && (fmvoices[voice].time < old_time ||
			    (fmvoices[voice].time == old_time &&
			     fmvoices[voice].patch >= N_INTR)))
	    {
	      old_time = fmvoices[voice].time;
	      old_voice = voice;
	    }
	}
    }

  return old_voice;
}


/*
 * Routine to find a free voice, if none free decide which note to kill
 */
static int fm_get_voice(int note, int secondary, int channel)
{
  int voice;
  int old_voice, old_time;
  int sec_voice = 0;
  int same_chn = 0;

  if (free_voices)
    return fm_pick_from_free(note, secondary, channel);

  /*
   * What? Out of voices?  Always choose secondary voices first.
   * First kill priority to secondaries on the same channel, then
   * to any voice on the same channel.  When there is choice pick the
   * longest playing voice
   */
  old_voice = 0;
  old_time  = curtime;

  for (voice = 0; voice < fm_sinfo.nr_voices; voice++)
    {
      /* secondary voices are prime targets */
      if (fmvoices[voice].secondary)
	{
	  if (!sec_voice || (!same_chn && channel == fmvoices[voice].channel))
	    {
	      sec_voice = 1;
	      old_voice = voice;
	      old_time = fmvoices[voice].time;
	      same_chn = (channel == fmvoices[voice].channel) ? 1 : 0;
	      continue;
	    }

	  /* if not on same channel and already got a same channel sec. skip */
	  if (same_chn && channel != fmvoices[voice].channel)
	    continue;

	  /* choose the oldest secondary */
	  if (fmvoices[voice].time < old_time)
	    {
	      old_voice = voice;
	      old_time = fmvoices[voice].time;
	    }
	  continue;
	}

      if (sec_voice)  /* already got a secondary skip the rest */
	continue;

      /* no secondary found yet, take the oldest playing note */
      if (same_chn && channel != fmvoices[voice].channel)
	continue;

      if (!same_chn && channel == fmvoices[voice].channel)
	{
	  same_chn = 1;
	  old_voice = voice;
	  old_time = fmvoices[voice].time;
	  continue;
	}

      if (fmvoices[voice].time < old_time)
	{
	  old_voice = voice;
	  old_time = fmvoices[voice].time;
	}
    }

#ifdef INSTALL_SIGH
  droppednotes++; 
  sigwinch_hdlr(0);
#endif
#ifdef FM_DEBUG
  printf("fm_get_voice: OOV, killing %s %s voice %d\n",
	 same_chn ? "same channel" : "",
	 fmvoices[old_voice].secondary ? "secondary" : "primary", old_voice);
#endif
  return old_voice;
}


static int fm_start_voice(int channel, int voice, int secondary,
			  int note, int velocity)
{
  const struct opl_instr *opl;
  int pri_patch, voice_patch;
  int valid_perc = 0;

  fmvoices[voice].active    = 1;
  fmvoices[voice].note      = note;
  fmvoices[voice].channel   = channel;
  fmvoices[voice].secondary = secondary;
  fmvoices[voice].time      = curtime;

  /*
   * there's an undocumented "feature" in some MUS files that send
   * notes > 81 on the percussion channel. We default to playing them on
   * whatever patch was set on channel 9 (usually 0)
   */
  if (channel == CHN_PERCUSSION && VALID_PERC(note))
    {
      pri_patch = PERC_OFFSET + note;
      valid_perc = 1;
    }
  else
    pri_patch = fmpatches[channel];

  voice_patch = (secondary) ? voice2_map[pri_patch] : pri_patch;
  opl = &fm_instruments[pri_patch];

  if (valid_perc)
    note = (opl->flags & FL_FIXED_PITCH) ? opl->note : 60;
  else if (voxware_octave_bug)
    note += 12;

  note += opl->patchdata[secondary].basenote;


#if 0
  /* FIXME: pitch fine-tuning of secondary voice - DOESN'T WORK */
  if (secondary)
    /*
     * The units of this are sub-semitone, seem to be a raw OPL frequency adj.
     * We could use the bender for this, but it seems like a lot of work
     */
    note += opl->finetune - 128;
#endif


  /* see if we need to set a new patch */
  if (fmvoices[voice].patch != voice_patch)
    {
      fmvoices[voice].patch = voice_patch;
      SEQ_SET_PATCH(seq_dev, voice, voice_patch);


#ifdef FM_DEBUG
      printf("fm_start_voice: setting patch #%d for voice %d assigned to channel %d\n",
	     voice_patch, voice, channel);
#endif
    }

  /*
   * NOTES: All voice settings are reset upon a patch change or
   * note-off event.  Thus, we must set all of these to the
   * current channel state before playing.
   *
   * the volume is a 14-bit quantity when the driver is operating
   * in direct voice mode instead of the usual 7-bit MIDI. We don't scale
   * the volume here since we use /dev/mixer to control FM-volume
   */

  SEQ_CONTROL(seq_dev, voice, CTL_MAIN_VOLUME, chanvol[channel]<<FMVOL_ADJ);
  SEQ_BENDER(seq_dev, voice, fm_bender[channel]);
  SEQ_CONTROL(seq_dev, voice, CTL_PAN, fm_pan[channel]);

#ifdef FM_DEBUG
  printf("fm_start_voice: starting %s voice %d (note %d, velocity %d)\n",
	 secondary ? "secondary" : "primary",
	 voice, note, velocity);
#endif

  SEQ_START_NOTE(seq_dev, voice, note, velocity);
  if (free_voices > 0)
    free_voices--;
  return (opl->flags & FL_DOUBLE_VOICE);
}

void note_on(int note, int channel, int velocity)
{
#ifdef SEQ_DEBUG
  printf("note_on(note=%d, channel=%d, velocity=%d)\n", note, channel, velocity);
#endif

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      SEQ_START_NOTE(seq_dev, channel, note, velocity);
      break;
#endif
    case USE_MIDI_SYNTH:
      SEQ_MIDIOUT(seq_dev, MIDI_NOTEON | channel);
      SEQ_MIDIOUT(seq_dev, note);
      SEQ_MIDIOUT(seq_dev, velocity);
      break;

    case USE_FM_SYNTH:
      {
	int has_voice2;
	int voice;

	voice = fm_get_voice(note, 0, channel);
	has_voice2 = fm_start_voice(channel, voice, 0, note, velocity);

	/* don't allocate secondaries if we have to kill a playing note */
	if (has_voice2 && has_opl3 && free_voices)
	  {
	    voice = fm_get_voice(note, 1, channel);
	    fm_start_voice(channel, voice, 1, note, velocity);
	  }
      }
    default:
      break;
    }
}

void pitch_bend(int channel, int value)
{
  int adjvalue;

#ifdef SEQ_DEBUG
  printf("pitch_bend: channel %d to %d\n", channel, value);
#endif

  /* adj. range to be 0x3fff-0x0000, with 0x2000 = center */
  adjvalue = value << 6;
  if (value & 128)
    adjvalue |= (value & 127) >> 1;

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      SEQ_BENDER(seq_dev, channel, adjvalue);
      break;
#endif
    case USE_MIDI_SYNTH:
      SEQ_MIDIOUT(seq_dev, MIDI_PITCH_BEND | channel);
      SEQ_MIDIOUT(seq_dev, adjvalue & 127);  /* LSB first */
      SEQ_MIDIOUT(seq_dev, adjvalue >> 7);
      break;

    case USE_FM_SYNTH:
    default:
      {
	int i;

	fm_bender[channel] = adjvalue;

	for (i = 0; i < fm_sinfo.nr_voices; i++)
	  if (fmvoices[i].channel == channel)
	    SEQ_BENDER(seq_dev, i, adjvalue);
      }
      break;
    }
}

void control_change(int controller, int channel, int value)
{
#ifdef SEQ_DEBUG
  printf("control_change: controller %d, channel %d, value %d\n",
	 controller, channel, value);
#endif

  /* intecept volume changes so we can adj. volume globally */
  if (controller == CTL_MAIN_VOLUME)
    {
      chanvol[channel] = value;
      value = value * volscale / 100;
    }

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      SEQ_CONTROL(seq_dev, channel, controller, value);
      break;
#endif
    case USE_MIDI_SYNTH:
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
      SEQ_MIDIOUT(seq_dev, controller);
      SEQ_MIDIOUT(seq_dev, value);
      break;

    case USE_FM_SYNTH:
      {
	switch(controller)
	  {
	  case CTL_PAN:
	    fm_pan[channel] = value;
	    break;
	  case CTL_MAIN_VOLUME:
	    /*
	     * the OPL3 driver won't change the volume (w/o tricks)
	     * of a playing note, so there's not point in
	     * searching through them and ajusting volumes.
	     *
	     * anything else is not supported AFAIK.
	     */
	    break;
	  }
      }
    default:
      return;
    }
}

void system_event(int event, int channel)
{
  /* map of MUS (10-14) system events to MIDI events */
  static const char map2midi[5] = {0x78, 0x7b, 0x7e, 0x7f, 0x79};

  if (event < MUS_SYS_ALL_SOUND_OFF || event > MUS_SYS_RESET)
    {
      unknevents++;
      return;
    }

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      switch (event)
	{
	case MUS_SYS_ALL_SOUND_OFF:
	case MUS_SYS_ALL_NOTES_OFF:
	  /*
	   * this is way too drastic!, I rather let the notes linger...
	   * too bad there's no way to turn off just one channel /w sustain
	   * AWE_TERMINATE_CHANNEL(seq_dev, channel);
	   */
	  return;
	  break;
	case MUS_SYS_RESET:
	  AWE_RESET_CONTROL(seq_dev, channel);
	  break;
	default:
	  /* what to do with the POLY and MONO mode changes? */
	  return;
	}
#endif
    case USE_MIDI_SYNTH:
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
      SEQ_MIDIOUT(seq_dev, map2midi[event - MUS_SYS_ALL_SOUND_OFF]);
      SEQ_MIDIOUT(seq_dev, 0);
      break;

    case USE_FM_SYNTH:
    default:  /* nothing to do here */
      return;
    }
}

void patch_change(int patch, int channel)
{
#ifdef SEQ_DEBUG
  printf("patch_change: patch %d to channel %d\n", patch, channel);
#endif

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      SEQ_SET_PATCH(seq_dev, channel, patch);
      break;
#endif
    case USE_MIDI_SYNTH:
      SEQ_MIDIOUT(seq_dev, MIDI_PGM_CHANGE | channel);
      SEQ_MIDIOUT(seq_dev, patch);
      break;

    case USE_FM_SYNTH:
      fmpatches[channel] = patch;
    default:
      return;
    }
}

void midi_wait(int wait_time)
{
  if (seqfd < 0) {
	  usleep(10000);
	  return;
  }
#ifdef SEQ_DEBUG
  printf("midi_wait: until ticks=%d\n", wait_time);
#endif
  ioctl(seqfd, SNDCTL_SEQ_SYNC);
  SEQ_WAIT_TIME(wait_time);
  SEQ_DUMPBUF();
}

static int curvol;

void midi_timer(mid_timer_t action)
{
  static int lastvol = -1;

  switch (action)
    {
    case MIDI_START:  /* start a new song */
      reset_midi();
      if (lastvol != -1)
	{
	  vol_change(lastvol);
	  lastvol = -1;
	}
      curtime = 0;
      SEQ_START_TIMER();
      break;

    case MIDI_STOP:  /* stop playing a song for good */
      reset_midi();
      if (lastvol != -1)
	{
	  vol_change(lastvol);
	  lastvol = -1;
	}
      break;

    case MIDI_RESUME:  /* resume playing a song */
      vol_change(lastvol);
      lastvol = -1;
      if (synth_type == USE_FM_SYNTH)
	{
	  int i;

	  /* make the FM timestamps sensical once curtime is brought to zero */
	  for (i = 0; i < fm_sinfo.nr_voices; i++)
	      fmvoices[i].time -= curtime;
	}
      curtime = 0;
      SEQ_START_TIMER();
      break;

    case MIDI_PAUSE:
      lastvol = curvol;
      vol_change(0);
      if (seqfd != -1) ioctl(seqfd, SNDCTL_SEQ_SYNC);
      break;
    }
}

void vol_change(int volume)
{
  int x;
  static const int linscale[16] = {0, 6, 13, 19, 25, 31, 38, 44, 50,
				   56, 63, 75, 81, 88, 94, 100};

  curvol = (volume < 0 ? 0 : (volume > 15 ? 15 : volume));
  volscale = linscale[curvol];

  switch (synth_type)
    {
#ifdef ENABLE_AWE
    case USE_AWE_SYNTH:
      for (x = 0; x < N_CHN; x++)
        SEQ_CONTROL(seq_dev, x, CTL_MAIN_VOLUME, chanvol[x] * volscale / 100);
      break;
#endif
    case USE_MIDI_SYNTH:
      for (x = 0; x < N_CHN; x++)
	{
	  int newvol = chanvol[x] * volscale / 100;

	  SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | x);
	  SEQ_MIDIOUT(seq_dev, CTL_MAIN_VOLUME);
	  SEQ_MIDIOUT(seq_dev, newvol);
	}
      break;

    case USE_FM_SYNTH:
      if (mix_fd != -1)
	{
	  int newvol = (volscale<<8) | volscale;

	  if (ioctl(mix_fd, SOUND_MIXER_WRITE_SYNTH, &newvol) == -1)
	    {
	      printf("%s: ioctl error on /dev/mixer\n", progname);
	      perror(progname);
	      close(mix_fd);
	      mix_fd = -1;
	    }
	}
    default:
      return;
    }
  SEQ_DUMPBUF();
}

#ifdef INSTALL_SIGH
/* send a SIGWINCH to get a snapshot of the FM-synth state */
static void sigwinch_hdlr(int snum __attribute__ ((unused)))
{
  int i;

  printf("\n%s: FM-state report\n", progname);

  switch (synth_type)
  {
  case USE_FM_SYNTH:
    puts("voice\tnote\tchannel\tvolume\tbender\tpan\tpatch\tsecondary   last use\n");
    for (i = 0; i < fm_sinfo.nr_voices; i++)
      {
	int channel = fmvoices[i].channel;

	if (channel != -1)
	  printf("%2d:\t%3d %s\t  %2d\t %3d\t0x%04x\t%3d\t %3d\t   %s\t     %6d\n",
		 i, fmvoices[i].note,
		 (fmvoices[i].active) ? "on" : "off",
		 channel, chanvol[channel],
		 fm_bender[channel],
		 fm_pan[channel], fmvoices[i].patch,
		 (fmvoices[i].patch >= N_INTR) ? "yes" : "no",
		 fmvoices[i].time);
	else
	  printf("%2d:\tunassigned\t\t\t\t\t\t     %6d\n", i, fmvoices[i].time);
      }

    printf("\n\tfree voices:\t\t%d\n", free_voices);

  default:
    break;
  }

  printf("\n\tMUS unknown events:\t%d\n"
	 "\tdropped notes:\t\t%d\n",
	 unknevents, droppednotes);
}
#endif
