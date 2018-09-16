/*************************************************************************
 *  playmus.c
 *
 *  Updated to lxdoom, added support for additional MUS file events,
 *  bugfixed and tidyuped the code
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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "musserver.h"
#include "sequencer.h"

int unknevents;
int timer_res;
int curtime;

void playmus(const struct MUSfile *song, int newsong, int looping, int fd)
{
  struct timeval timeout;
  fd_set readfds;

  static int lastvol[N_CHN];
  static unsigned char *ptr;

  int last = 0;
  int controleqv;
  unsigned char event;
  unsigned char eventtype;
  unsigned char channelnum;
  unsigned char notenum;
  unsigned char pitchwheel;
  unsigned char controlnum;
  unsigned char controlval;

  if (newsong)
    ptr = song->musdata;

  do {
    event = *ptr++;
    channelnum = event & 15;

    /* map channel 16 to channel 10 (drum channel) */
    if (channelnum >= CHN_PERCUSSION)
      {
	channelnum++;
	if (channelnum == MUS_CHN_PERCUSSION + 1)
	  channelnum = CHN_PERCUSSION;
      }

    eventtype = (event >> 4) & 7;
    last = event & 128;

    switch (eventtype)
      {
      case MUS_EVENT_NOTE_OFF:
	notenum = *ptr++;
	note_off(notenum & 127, channelnum, lastvol[channelnum]);
	break;

      case MUS_EVENT_NOTE_ON:
	notenum = *ptr++;
	if (notenum & 128)
	  {
	    unsigned char notevol = *ptr++;
	    lastvol[channelnum] = notevol & 127;
	  }
	note_on(notenum & 127, channelnum, lastvol[channelnum]);
	break;

      case MUS_EVENT_BENDER:
	pitchwheel = *ptr++;
	pitch_bend(channelnum, pitchwheel & 127);
	break;

      case MUS_EVENT_SYS:
	event = *ptr++;
	system_event(event, channelnum);
	break;

      case MUS_EVENT_CTL_CHANGE:
	controlnum = *ptr++ & 127;
	controlval = *ptr++ & 127;
	controleqv = -1;
	switch (controlnum)
	  {
	  case MUS_CTL_PATCH_CHG: /* patch change */
	    patch_change(controlval, channelnum);
	    break;
	  case MUS_CTL_BNK_SEL:
	    controleqv = CTL_BANK_SELECT;
	    break;
	  case MUS_CTL_MOD_POT:
	    controleqv = CTL_MODWHEEL;
	    break;
	  case MUS_CTL_MAIN_VOL:
	    controleqv = CTL_MAIN_VOLUME;
	    break;
	  case MUS_CTL_PAN_BAL:
	    controleqv = CTL_PAN;
	    break;
	  case MUS_CTL_EXPR_POT:
	    controleqv = CTL_EXPRESSION;
	    break;
	  case MUS_CTL_CHORUS:
	    controleqv = CTL_CHORUS_DEPTH;
	    break;
	  case MUS_CTL_SUSTAIN:
	    controleqv = CTL_SUSTAIN;
	    break;
	  case MUS_CTL_SOFT_PEDAL:
	    controleqv = CTL_SOFT_PEDAL;
	    break;
	  case MUS_CTL_REVRB:
	    controleqv = CTL_EXT_EFF_DEPTH;
	    break;
	  default:
	    unknevents++;
	    break; 
	  }
	if (controleqv != -1)
	  control_change(controleqv, channelnum, controlval);
	break;

      case MUS_EVENT_SCORE_END:
        if (!looping)
          return;
        else
          {
	    ptr = song->musdata;
	    midi_timer(MIDI_START);
	    last = 0;
          }
	break;

      default:	/* unknown */
	unknevents++;
	break;
      }

    if (last)	/* next data portion is time data */
      {
	unsigned char tmpch;
	int dticks = 0;

	do
	  {
	    tmpch = *ptr++;
	    dticks = (dticks << 7) | (tmpch & 127);
	  }
	while (tmpch & 128);
	curtime += dticks;
	midi_wait(curtime*timer_res/TICS_PER_SECOND);
      }

    /*
     * check if a new command has arrived
     */
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    bzero(&timeout, sizeof(timeout));
  } while (select(fd+1, &readfds, NULL, NULL, &timeout) == 0);
}
