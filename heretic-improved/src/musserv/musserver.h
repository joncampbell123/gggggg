/*************************************************************************
 *  musserver.h
 *
 *  update to lxdoom
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

#ifdef __linux__
#  include <sys/soundcard.h>
#  include <linux/version.h>
#elif defined(__FreeBSD__)
#  include <machine/soundcard.h>
#  include <awe_voice.h>
#endif

#ifndef __PACKED__
#define __PACKED__
#endif

extern char *progname;
extern int verbose;
extern int unknevents;
extern int timer_res;          /* sequencer timer resolution (tics/sec) */
extern int curtime;            /* current song time in sequencer tics   */
extern int voxware_octave_bug; /* increase octave by 1 when using FM    */
extern int dumpmus;            /* dump to disk music and FM-patches     */

#define DFL_VOL 12      /* default volume when standalone 1-15 (0 == off) */
#define CLEAN_EXIT   0
#define IO_ERROR     1
#define MISC_ERROR   2  /* doesn't call perror() */

#define MUS_LOOP     1
#define MUS_NOLOOP   0
#define MUS_NEWSONG  1
#define MUS_SAMESONG 0

#define NO_PATCH_NEEDED 0
#define PATCH_NEEDED    1

/* number of instruments   */
#define N_INTR          175
/* number of MIDI channels */
#define N_CHN            16


typedef char (instr_nam)[32];

/*
 * Note: gcc attribute "packed" is used to make sure the compiler doesn't
 * try to optimize the structures by aligning elements to word boundaries, etc.
 */

struct OPL2instrument {
  unsigned char trem_vibr_1;    /* OP 1: tremolo/vibrato/sustain/KSR/multi */
  unsigned char att_dec_1;      /* OP 1: attack rate/decay rate */
  unsigned char sust_rel_1;     /* OP 1: sustain level/release rate */
  unsigned char wave_1;         /* OP 1: waveform select */
  unsigned char scale_1;        /* OP 1: key scale level */
  unsigned char level_1;        /* OP 1: output level */
  unsigned char feedback;       /* feedback/AM-FM (both operators) */
  unsigned char trem_vibr_2;    /* OP 2: tremolo/vibrato/sustain/KSR/multi */
  unsigned char att_dec_2;      /* OP 2: attack rate/decay rate */
  unsigned char sust_rel_2;     /* OP 2: sustain level/release rate */
  unsigned char wave_2;         /* OP 2: waveform select */
  unsigned char scale_2;        /* OP 2: key scale level */
  unsigned char level_2;        /* OP 2: output level */
  unsigned char unused;
  short         basenote;       /* base note offset */
}  __PACKED__ ;


struct opl_instr {
	unsigned short        flags;
#define FL_FIXED_PITCH  0x0001          // note has fixed pitch (drum note)
#define FL_UNKNOWN      0x0002          // ??? (used in instrument #65 only)
#define FL_DOUBLE_VOICE 0x0004          // use two voices instead of one

	unsigned char         finetune;
	unsigned char         note;
	struct OPL2instrument patchdata[2];
}  __PACKED__ ;


typedef struct OPLfile {
     unsigned char     id[8];
     struct opl_instr *opl_instruments;
     instr_nam        *instrument_nam;
} OPLFILE;


struct MUSheader {
	char           id[4];          // identifier "MUS" 0x1A
	unsigned short scoreLen;
	unsigned short scoreStart;
	unsigned short channels;       // count of primary channels
	unsigned short sec_channels;   // count of secondary channels
	unsigned short instrCnt;
	unsigned short dummy;
}  __PACKED__ ;


typedef struct MUSfile {
	struct MUSheader  header;
        unsigned short   *instr_pnum;
	unsigned char    *musdata;
} MUSFILE;

typedef enum  {
  USE_AWE_SYNTH, USE_MIDI_SYNTH, USE_FM_SYNTH,
  USE_BEST_SYNTH, LIST_ONLY_SYNTH
} synthdev_t;

typedef enum {
  MIDI_START, MIDI_STOP, MIDI_PAUSE, MIDI_RESUME
} mid_timer_t;


extern void warning(int error, const char *mesg);
extern void cleanup(int status, const char *msg) __attribute__ ((noreturn));
extern const MUSFILE *readmus(FILE *fp);
extern const OPLFILE *read_genmidi(FILE *fp);
extern void playmus(const struct MUSfile *mus, int newsong,
		    int looping, int monitor_fd);
