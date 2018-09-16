
#ifndef __I_IPX__
#define __I_IPX__

#include "doomdef.h"

/* Called by I_InitNetwork */

extern void I_InitIPX (void);

/* Called by I_NetCmd. */

extern void IPXPacketSend (void);
extern int IPXPacketGet (void);


#endif
