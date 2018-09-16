/*
 *-----------------------------------------------------------------------------
 *
 * $Log: wadread.h,v $
 *
 * Revision 2.0  1999/01/24 00:52:12  Andre Wertmann
 * changed to work with Heretic 
 *
 * Revision 1.5  1998/04/24 23:35:03  chris
 * read_pwads() prototype
 *
 * Revision 1.4  1998/04/24 21:07:45  chris
 * ID's released version
 *
 * Revision 1.3  1997/01/30 19:54:23  b1
 * Final reformatting run. All the remains (ST, W, WI, Z).
 *
 * Revision 1.2  1997/01/21 19:00:10  b1
 * First formatting run:
 *  using Emacs cc-mode.el indentation for C++ now.
 *
 * Revision 1.1  1997/01/19 17:22:52  b1
 * Initial check in DOOM sources as of Jan. 10th, 1997
 *
 *
 * DESCRIPTION:
 *	WAD and Lump I/O, the second.
 *	This time for soundserver only.
 *	Welcome to Department of Redundancy Department.
 *	 (Yeah, I said that elsewhere already).
 *	Note: makes up for a nice w_wad.h.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __WADREAD_H__
#define __WADREAD_H__


/* Helper function */
char* accesswad(char* wadname);

/*
 *  Opens the wadfile specified.
 * Must be called before any calls to  loadlump() or getsfx().
 */

void openwad(char* wadname);

/*
 *  Gets a sound effect from the wad file.  The pointer points to the
 *  start of the data.  Returns a 0 if the sfx was not
 *  found.  Sfx names should be no longer than 6 characters.  All data is
 *  rounded up in size to the nearest MIXBUFFERSIZE and is padded out with
 *  0x80's.  Returns the data length in len.
 */

void* getsfx( char* sfxname, int* len );

/*
 *  Gets sound data from user wads. Idea from musserver-1.22
 */

void read_pwads(void);

#endif
