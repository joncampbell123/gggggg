
#ifndef __I_UDP__
#define __I_UDP__

#include "doomdef.h"

/* Called by I_InitNetwork */

extern void I_InitUDP (void);

/* Called by I_NetCmd. */

extern void UDPPacketSend (void);
extern void UDPPacketGet (void);


#endif
