/*
 *-----------------------------------------------------------------------------
 *
 * $Log: soundsrv.h,v $
 *
 * Revision 2.0  1999/01/24 00:52:12  Andre Wertmann
 * changed to work with Heretic 
 *
 * Revision 1.3  1997/01/29 22:40:44  b1
 * Reformatting, S (sound) module files.
 *
 * Revision 1.2  1997/01/21 19:00:07  b1
 * First formatting run:
 *  using Emacs cc-mode.el indentation for C++ now.
 *
 * Revision 1.1  1997/01/19 17:22:50  b1
 * Initial check in DOOM sources as of Jan. 10th, 1997
 *
 *
 * DESCRIPTION:
 *	UNIX soundserver, separate process. 
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __SNDSERVER_H__
#define __SNDSERVER_H__

#define SAMPLECOUNT	512
#define MIXBUFFERSIZE	(SAMPLECOUNT*2*2)
#define SPEED		11025


void I_InitMusic(void);

void I_InitSound( int samplerate, int samplesound );

void I_SubmitOutputBuffer( void* samples, int samplecount );

void I_ShutdownSound(void);
void I_ShutdownMusic(void);

#endif
