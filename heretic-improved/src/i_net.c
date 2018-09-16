
#include <stdlib.h>
#include <assert.h>

#include "doomdef.h"

#ifdef IPX_PROTOCOL
#include "i_ipx.h"
#endif
#ifdef UDP_PROTOCOL
#include "i_udp.h"
#endif


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

#ifdef IPX_PROTOCOL
  
  /* check, whether IPX network play is wanted */
  
  i = M_CheckParm("-ipx");
  if (i)
    {
      netsend = IPXPacketSend;
      netget = (void (*)(void)) IPXPacketGet; /* IPXPacketGet() normally returns an int */
      netgame = true;
      I_InitIPX();
      return;
    }
  
#endif
  
#ifdef UDP_PROTOCOL
  
  /*
   * parse UDP/IP network game options,
   *  -net <consoleplayer> <host> <host> ...
   */
  i = M_CheckParm ("-net");
  if (i)
    {
      netsend = UDPPacketSend;
      netget = UDPPacketGet;
      netgame = true;
      I_InitUDP();
      return;
    }
#endif
  
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

