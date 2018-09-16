
#include <stdlib.h>
#include <assert.h>

#include "doomdef.h"


void (* netget)(void);
void (* netsend)(void);

/*
 * I_InitNetwork
 */
void I_InitNetwork (void)
{
  int i;
  
  doomcom = malloc (sizeof (*doomcom) );
  assert(doomcom);
  memset (doomcom, 0, sizeof(*doomcom) );
  netbuffer = &doomcom->data;
  
  /*  
   * shared parameters (IPX/UDP)
   * set up for network
   */
  i = M_CheckParm ("-dup");
  if (i && i< myargc-1)
    {
      doomcom->ticdup = myargv[i+1][0]-'0';
      if (doomcom->ticdup < 1)
	doomcom->ticdup = 1;
      if (doomcom->ticdup > 9)
	doomcom->ticdup = 9;
    }
  else
    doomcom-> ticdup = 1;
  
  if (M_CheckParm ("-extratic"))
    doomcom-> extratics = 1;
  else
    doomcom-> extratics = 0;
  /* shared parameters end */
  
  /* single player game */
  netgame = false;
  doomcom->id = DOOMCOM_ID;
  doomcom->numplayers = doomcom->numnodes = 1;
  doomcom->deathmatch = false;
  doomcom->consoleplayer = 0;
  
  /*  
   * this was needed, additionally (from old I_InitNetwork())
   * doomcom->ticdup=1;
   * doomcom->extratics=0;
   */
  return;
}


void I_NetCmd (void)
{
  if (doomcom->command == CMD_SEND)
    {
      netsend ();
    }
  else if (doomcom->command == CMD_GET)
    {
      netget ();
    }
  else
    I_Error ("Bad net cmd: %i\n",doomcom->command);
}

