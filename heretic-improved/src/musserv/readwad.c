/*************************************************************************
 *  readwad.c
 *
 *  Updated to lxdoom and several improvements (see NOTES)
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
#include <string.h>
#include <errno.h>
#include "musserver.h"
#include "sequencer.h"


static const unsigned char mus_id[4] = {'M', 'U', 'S', 0x1a};
static MUSFILE mus;
static const unsigned char opl_id[8] = {"#OPL_II#"};
static OPLFILE opl;

/*
 * dump MUS file to disk
 */
static inline void dump_mus(void)
{
  static int nsong;
  FILE *song;
  char buf[16];

  sprintf(buf, "song%02d.mus", nsong++);
  song = fopen(buf, "w");
  if (song)
    {
      fwrite(&mus.header, sizeof(struct MUSheader), 1, song);
      fwrite(mus.instr_pnum, sizeof(unsigned short), mus.header.instrCnt, song);
      fwrite(mus.musdata, 1, mus.header.scoreLen, song);
      fclose(song);
      if (verbose)
	      printf("%s: wrote \"%s\" MUS file\n", progname, buf);
    }
}

/*
 * dump raw OPL midi information
 */
static inline void dump_opl(void)
{
  FILE *fmid;
  const char rmidp[] = "raw_midi.patch";

  fmid = fopen(rmidp, "w");
  if (fmid)
    {
      fwrite(opl.id, sizeof(opl.id), 1, fmid);
      fwrite(opl.opl_instruments, sizeof(struct opl_instr), N_INTR, fmid);
      fwrite(opl.instrument_nam, sizeof(instr_nam), N_INTR, fmid);
      if (verbose)
	      printf("%s: wrote \"%s\" OPL3 patch file\n", progname, rmidp);
      fclose(fmid);
    }
}

/*
 * read MUS file
 */
const MUSFILE *readmus(FILE *fp)
{
  static int max_score, max_instr;

  errno = 0;
  if (fread(&mus.header, sizeof(mus.header), 1, fp) != 1)
    cleanup(IO_ERROR, "can't read MUS file header");

  /* MUS ID check */
  if (memcmp(mus.header.id, mus_id, sizeof(mus_id)))
    cleanup(MISC_ERROR, "MUS file ID check failed");

  /* Allocate new memory if required */
  if (max_instr < mus.header.instrCnt)
    {
      mus.instr_pnum = (unsigned short *) realloc(mus.instr_pnum,
						  sizeof(unsigned short) *
						  mus.header.instrCnt);
      max_instr = mus.header.instrCnt;
    }
  if (max_score < mus.header.scoreLen)
    {
      mus.musdata = (unsigned char *) realloc(mus.musdata, mus.header.scoreLen);
      max_score = mus.header.scoreLen;
    }
  if (!mus.instr_pnum || !mus.musdata)
    cleanup(MISC_ERROR, "couldn't allocate memory for MUS file");

  /* read the instruments' patch numbers */
  if (fread(mus.instr_pnum, sizeof(unsigned short), mus.header.instrCnt, fp)
      != mus.header.instrCnt)
    cleanup(IO_ERROR, "can't read MUS instrument data");

  /* read the music data */
  if (fread(mus.musdata, 1, mus.header.scoreLen, fp) != mus.header.scoreLen)
    cleanup(IO_ERROR, "can't read MUS music data");

  if (fp != stdin)
    fclose(fp);
  else if (dumpmus)
    dump_mus();

  return &mus;
}

/*
 * read OPL GENMIDI patches
 */
const OPLFILE *read_genmidi(FILE *fp)
{
  errno = 0;
  /* read the ID */
  if (fread(opl.id, sizeof(opl_id), 1, fp) != 1)
    cleanup(IO_ERROR, "Can't read GENMIDI ID");

  /* verify the ID */
  if (memcmp(opl.id, opl_id, sizeof(opl_id)))
    cleanup(MISC_ERROR, "GENMIDI file failed id check");

  /* allocate memory */
  opl.opl_instruments=(struct opl_instr *)malloc(sizeof(struct opl_instr)*N_INTR);
  opl.instrument_nam = (instr_nam *) malloc(sizeof(instr_nam)*N_INTR);
  if (!opl.opl_instruments || !opl.instrument_nam)
    cleanup(MISC_ERROR, "couldn't allocate memory for instrument data");

  /* read the data */
  if (fread(opl.opl_instruments, sizeof(struct opl_instr), N_INTR, fp)!=N_INTR)
    cleanup(IO_ERROR, "can't read instrument patch data");

  if (fread(opl.instrument_nam, sizeof(instr_nam), N_INTR, fp) != N_INTR)
    cleanup(IO_ERROR, "can't read instrument names");

  if (fp != stdin)
    fclose(fp);
  else if (dumpmus)
    dump_opl();

  return &opl;
}
