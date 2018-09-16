/* GSI - general sound interface
 *
 * sound.c
 *
 * By W.H.Scholten 1998
 *
 * License for this file: Public domain.
 */

/* export GSI_DEBUG_LEVEL=n for debug output */


#define DEBUG_PREFIX "GSI_"

#ifndef DISABLE_SOUND

#if  ! USE_NAS
#include <gsi/gsi_interface.c>
#endif

#if  USE_NAS
#include <gsi/gsi_nas_interface.c>
#endif

#else
#include <gsi/gsi_interface_disabled.c>

#endif
