/*
 *
 * $Log: doomtype.h,v $
 * Revision 1.4  1998/10/16 06:33:04  chris
 * moved FALSE and TRUE definition outside of '#ifndef HAVE_VALUES_H'
 * block (was slipped in there by mistake)
 *
 * Revision 1.4  1999/01/13 20:47:xx  Andre` Werthmann
 * changed Heretic-source to work under Linux/GGI
 *
 * Revision 1.3  1998/10/14 23:17:42  chris
 * introduced __32BIT__ and __64BIT__ defines
 *
 * Revision 1.2  1998/09/27 22:37:43  chris
 * values.h isn't always present
 *
 * Revision 1.1  1998/09/27 22:18:42  chris
 * Initial revision
 *
 *
 * DESCRIPTION:
 *	Simple basic typedefs, isolated here to make it easier
 *	 separating modules.
 */


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum {false, true} boolean;
typedef unsigned char byte;
#endif

/* Predefined with some OS. */
#ifdef HAVE_VALUES_H
#include <values.h>
#ifndef MININT
#define MININT ((-(MAXINT)) - 1)
#endif
#else

#if defined(__alpha__) || defined(_LONGLONG) || defined(__64BIT__)

/* 64bit defines */

#define MAXCHAR		((char)0x7f)
#define MAXSHORT	((short)0x7fff)

/* Max pos 32-bit int. */
#define MAXINT		((int)0x7fffffff)
#define MAXLONG		((long)0x7fffffffffffffff)
#define MINCHAR		((char)0x80)
#define MINSHORT	((short)0x8000)

/* Max negative 32-bit integer. */
#define MININT		((int)0x80000000)
#define MINLONG		((long)0x8000000000000000)

#endif /* 64bit */

#if defined(__32BIT__)

/* 32bit defines */

#define MAXCHAR		((char)0x7f)
#define MAXSHORT	((short)0x7fff)

/* Max pos 32-bit int. */
#define MAXINT		((int)0x7fffffff)
#define MAXLONG		((long)0x7fffffff)
#define MINCHAR		((char)0x80)
#define MINSHORT	((short)0x8000)

/* Max negative 32-bit integer. */
#define MININT		((int)0x80000000)
#define MINLONG		((long)0x80000000)

#else /* previous: 32bit, next: error */

/* safety guard */
#ifndef __64BIT__
#error not 32bit and not 64bit machine!?
#endif

#endif /* end of #if defined(__32BIT__) chunk */
#endif /* #ifndef HAVE_VALUES_H */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* Structure packing */
#ifndef __PACKED__
#define __PACKED__
#endif

#include <sys/types.h>

#endif   /* __DOOMTYPE__ */

