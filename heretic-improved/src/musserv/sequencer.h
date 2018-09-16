/*************************************************************************
 *  sequencer.h
 *
 *  new file - update to lxdoom (see NOTES)
 *  Copyright (C) 1999 Rafael Reilova (rreilova@ececs.uc.edu)
 *
 *  Original code
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

#define FMVOL_ADJ     5      // can use 1-7 (7 = full scale, but may distort)

#define CHN_PERCUSSION         9
#define MUS_CHN_PERCUSSION    15
#define PERC_OFFSET    (128 - 35)

#define MUS_EVENT_NOTE_OFF     0
#define MUS_EVENT_NOTE_ON      1
#define MUS_EVENT_BENDER       2
#define MUS_EVENT_SYS          3
#define MUS_EVENT_CTL_CHANGE   4
/* event #5  ? */
#define MUS_EVENT_SCORE_END    6
/* event #7  ? */

#define MUS_SYS_ALL_SOUND_OFF 10
#define MUS_SYS_ALL_NOTES_OFF 11
#define MUS_SYS_MONO          12
#define MUS_SYS_POLY          13
#define MUS_SYS_RESET         14

#define MUS_CTL_PATCH_CHG      0
#define MUS_CTL_BNK_SEL        1
#define MUS_CTL_MOD_POT        2
#define MUS_CTL_MAIN_VOL       3
#define MUS_CTL_PAN_BAL        4
#define MUS_CTL_EXPR_POT       5
#define MUS_CTL_REVRB          6
#define MUS_CTL_CHORUS         7
#define MUS_CTL_SUSTAIN        8
#define MUS_CTL_SOFT_PEDAL     9

/* Std. tempo for all Doom/Heretic */
#define TICS_PER_SECOND 140

#define PAN_CENTER     64
#define MAIN_VOL_NRML 100
#define MAIN_VOL_MAX  127
#define PITCH_CENTER  0x2000

/*
 * state of FM-voices
 */
struct fm_voice
{
  int active;
  int note;
  int channel;
  int secondary;
  int patch;
  int time;
};

/*
 * MIDI sequencer routines
 */
extern void note_off(int note, int channel, int volume);
extern void note_on(int note, int channel, int volume);
extern void pitch_bend(int channel, int value);
extern void control_change(int controller, int channel, int value);
extern void patch_change(int patch, int channel);
extern void midi_wait(int wait_time);
extern void vol_change(int volume);
extern void system_event(int event_code, int channel);

/*
 * MIDI setup/control routines
 */
extern void fmload(const struct opl_instr *);
extern void cleanup_midi(void);
extern void midi_timer(mid_timer_t action);
extern int midi_setup(synthdev_t pref_dev, int num_dev);

/*
 * shared MIDI init/sequencer stuff
 */
extern int mix_fd;
extern struct synth_info fm_sinfo;
extern synthdev_t synth_type;
extern int seqfd, seq_dev, has_opl3;
extern struct fm_voice *fmvoices;
