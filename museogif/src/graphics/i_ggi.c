/* Version 1.3 - 990522
******************************************************************************

   LibGGI display code for Doom and Heretic

   Copyright (C) 1999	Marcus Sundberg [marcus@ggi-project.org]
  
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

/* Comment this out to disable support for FPS meter */
#define SHOW_FPS

/* Uncomment this is you have a Linux Heretic prior to 1.0beta1 */
/*
#define NO_FILTERING
*/

#include <stdlib.h>
#include <ctype.h>
#include <ggi/ggi.h>

#ifdef I_GGI_DOOM
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "r_draw.h"
#endif
#ifdef I_GGI_HERETIC
void R_InitBuffer (int width, int height);
#endif

#include "doomdef.h"

#if defined(I_GGI_DOOM)
#define SCREEN0_PTR	(screens[0])
#elif defined(I_GGI_HERETIC)
#define SCREEN0_PTR	(screen)
#else
#error You must define I_GGI_DOOM or I_GGI_HERETIC!
#endif

/* Needed by reset_framecounter() */
static ggi_visual_t ggivis=NULL;

#ifdef SHOW_FPS
#include <unistd.h>
#include <sys/time.h>

static struct timeval	starttime;
static long		totalframes;
static int		showfps = 0;

static void reset_framecounter(void)
{
	ggi_color black = { 0x0, 0x0, 0x0 };
	ggi_color white = { 0xffff, 0xffff, 0xffff };

	/* Set text colors */
	ggiSetGCForeground(ggivis, ggiMapColor(ggivis, &white));
	ggiSetGCBackground(ggivis, ggiMapColor(ggivis, &black));

	totalframes = 0;
	gettimeofday(&starttime, NULL);
}
#endif

static int     lastmousex = 0;
static int     lastmousey = 0;
boolean        mousemoved = false;
int            buttonstate=0;

static int     realwidth, realheight;
static int     doublebuffer;
static int     scale;
static int     stride;
static int     pixelsize;

static const ggi_directbuffer *dbuf1, *dbuf2;
static int     usedbuf, havedbuf;
static void   *frameptr[2] = { NULL, NULL };
static void   *oneline = NULL;
static void   *palette = NULL;
static int     curframe = 0;
static int     modexrefresh=0;
static int     dontdraw = 0;


static inline void
do_scale8(int xsize, int ysize, uint8 *dest, uint8 *src)
{
	int i, j, destinc = stride*2-xsize*2;
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; /* i is incremented below */) {
			register uint32 pix1 = src[i++], pix2 = src[i++];
#ifdef GGI_LITTLE_ENDIAN
			*((uint32 *) (dest + stride))
				= *((uint32 *) dest)
				= (pix1 | (pix1 << 8)
				   | (pix2 << 16) | (pix2 << 24));
#else
			*((uint32 *) (dest + stride))
				= *((uint32 *) dest)
				= (pix2 | (pix2 << 8)
				   | (pix1 << 16) | (pix1 << 24));
#endif
			dest += 4;
		}
		dest += destinc;
		src += xsize;
	}
}

static inline void
do_scale16(int xsize, int ysize, uint8 *dest, uint8 *src)
{
	int i, j, destinc = stride*2-xsize*4;
	uint16 *palptr = palette;
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; /* i is incremented below */) {
			register uint32 pixel = palptr[src[i++]];
			*((uint32 *) (dest + stride))
				= *((uint32 *) dest)
				= pixel | (pixel << 16);
			dest += 4;
		}
		dest += destinc;
		src += xsize;
	}
}

static inline void
do_scale32(int xsize, int ysize, uint8 *dest, uint8 *src)
{
	int i, j, destinc = stride*2-xsize*8;
	uint32 *palptr = palette;
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; /* i is incremented below */) {
			register uint32 pixel = palptr[src[i++]];
			*((uint32 *) (dest + stride))
				= *((uint32 *) (dest)) = pixel;
			dest += 4;
			*((uint32 *) (dest + stride))
				= *((uint32 *) (dest)) = pixel;
			dest += 4;
		}
		dest += destinc;
		src += xsize;
	}
}


static inline void
do_copy8(int xsize, int ysize, uint8 *dest, uint8 *src)
{
	int i, j;
	uint8 *palptr = palette;
	
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; i++) {
			dest[i] = palptr[src[i]];
		}
		dest += stride;
		src += xsize;
	}
}

static inline void
do_copy16(int xsize, int ysize, uint16 *dest, uint8 *src)
{
	int i, j, destinc = stride/2;
	uint16 *palptr = palette;
	
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; i++) {
			dest[i] = palptr[src[i]];
		}
		dest += destinc;
		src += xsize;
	}
}

static inline void
do_copy32(int xsize, int ysize, uint32 *dest, uint8 *src)
{
	int i, j, destinc = stride/4;
	uint32 *palptr = palette;
	
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; i++) {
			dest[i] = palptr[src[i]];
		}
		dest += destinc;
		src += xsize;
	}
}


int key(int label, int sym)
{
	int rc=0;
	switch(label) {
	case GIIK_CtrlL:  case GIIK_CtrlR:  rc=KEY_RCTRL; 	break;
	case GIIK_ShiftL: case GIIK_ShiftR: rc=KEY_RSHIFT;	break;
	case GIIK_MetaL:  case GIIK_MetaR:
	case GIIK_AltL:   case GIIK_AltR:   rc=KEY_RALT;	break;
		
	case GIIUC_BackSpace:	rc = KEY_BACKSPACE;	break;
	case GIIUC_Escape:	rc = KEY_ESCAPE;	break;
#ifdef I_GGI_HERETIC
	case GIIK_Delete:	rc = KEY_DELETE;	break;
	case GIIK_Insert:	rc = KEY_INSERT;	break;
	case GIIK_PageUp:	rc = KEY_PAGEUP;	break;
	case GIIK_PageDown:	rc = KEY_PAGEDOWN;	break;
	case GIIK_Home:	rc = KEY_HOME;		break;
	case GIIK_End:	rc = KEY_END;		break;
#endif /* I_GGI_HERETIC */
	case GIIUC_Tab:	rc = KEY_TAB;		break;
	case GIIK_Up:	rc = KEY_UPARROW;	break;
	case GIIK_Down:	rc = KEY_DOWNARROW;	break;
	case GIIK_Left:	rc = KEY_LEFTARROW;	break;
	case GIIK_Right:rc = KEY_RIGHTARROW;	break;
	case GIIK_Enter:rc = KEY_ENTER;		break;
	case GIIK_F1:	rc = KEY_F1;		break;
	case GIIK_F2:	rc = KEY_F2;		break;
	case GIIK_F3:	rc = KEY_F3;		break;
	case GIIK_F4:	rc = KEY_F4;		break;
	case GIIK_F5:	rc = KEY_F5;		break;
	case GIIK_F6:	rc = KEY_F6;		break;
	case GIIK_F7:	rc = KEY_F7;		break;
	case GIIK_F8:	rc = KEY_F8;		break;
	case GIIK_F9:	rc = KEY_F9;		break;
	case GIIK_F10:	rc = KEY_F10;		break;
	case GIIK_F11:	rc = KEY_F11;		break;
	case GIIK_F12:	rc = KEY_F12;		break;
	case GIIK_Pause:rc = KEY_PAUSE;		break;

	default:
		/* Must use label here, or it won't work when shift is down */
		if ((label >= '0' && label <= '9') ||
		    (label >= 'A' && label <= 'Z') ||
		    label == '.' ||
		    label == ',') {
			/* We want lowercase */
			rc = tolower(label);
		} else if (sym < 256) {
			/* ASCII key - we want those */
			rc = sym;
			switch (sym) {
				/* Some special cases */
			case '+': rc = KEY_EQUALS;	break;
			case '-': rc = KEY_MINUS;
			default:			break;
			}
		}
	}
	return rc;
}

void I_ShutdownGraphics(void)
{
	if (ggivis != NULL) {
		if (!usedbuf) {
			free(SCREEN0_PTR);
		}
		if (oneline) {
			free(oneline);
			oneline = NULL;
		}
		if (palette) {
			free(palette);
			palette = NULL;
		}
		ggiClose(ggivis);
		ggivis = NULL;
	}
	ggiExit();
}


void I_StartFrame (void)
{
}

void I_GetEvent(void)
{
	event_t event;
	int nev;
	ggi_event ev;
	struct timeval t = {0,0};
	ggi_event_mask mask;

	mask = ggiEventPoll(ggivis, emAll, &t);
	if (!mask) return;
	
	nev = ggiEventsQueued(ggivis, mask);

	while (nev) {
		ggiEventRead(ggivis, &ev, mask);
		switch (ev.any.type) {
		case evKeyPress:
			event.type = ev_keydown;
			event.data1 = key(ev.key.label,ev.key.sym);
#ifdef SHOW_FPS
			if (event.data1 == KEY_BACKSPACE &&
			    gamestate == GS_LEVEL) {
				/* Toggle and reset the FPS counter */
				showfps = !showfps;
				reset_framecounter();
			}
#endif
			D_PostEvent(&event);
			break;
		case evKeyRelease:
			event.type = ev_keyup;
			event.data1 = key(ev.key.label,ev.key.sym);
			D_PostEvent(&event);
			break;
		case evPtrButtonPress:
			event.type = ev_mouse;
			buttonstate = event.data1 = 
				buttonstate | 1<<(ev.pbutton.button-1);
			event.data2 = event.data3 = 0;
			D_PostEvent(&event);
			break;
		case evPtrButtonRelease:
			event.type = ev_mouse;
			buttonstate = event.data1 = 
				buttonstate ^ 1<<(ev.pbutton.button-1);
			event.data2 = event.data3 = 0;
			D_PostEvent(&event);
			break;
		case evPtrAbsolute:
			event.type = ev_mouse;
			event.data1 = buttonstate;
			event.data2 = (ev.pmove.x - lastmousex) << 2;
			event.data3 = (lastmousey - ev.pmove.y) << 2;
			
			if (event.data2 || event.data3) {
				lastmousex = ev.pmove.x;
				lastmousey = ev.pmove.y;
				if (ev.pmove.x != screenwidth/2 &&
				    ev.pmove.y != screenheight/2) {
					D_PostEvent(&event);
					mousemoved = false;
				} else {
					mousemoved = true;
				}
			}
			break;
		case evPtrRelative:
			event.type = ev_mouse;
			event.data1 = buttonstate;
			event.data2 = ev.pmove.x << 2;
			event.data3 = -ev.pmove.y << 2;
			
			if (event.data2 || event.data3) {
				lastmousex += ev.pmove.x;
				lastmousey += ev.pmove.y;
				if (ev.pmove.x != screenwidth/2 &&
				    ev.pmove.y != screenheight/2) {
					D_PostEvent(&event);
					mousemoved = false;
				} else {
					mousemoved = true;
				}
			}
			break;
		case evCommand:
			if (ev.cmd.code == GGICMD_REQUEST_SWITCH) {
				ggi_cmddata_switchrequest *data = (void*)
					ev.cmd.data;
				if (data->request == GGI_REQSW_UNMAP) {
					ggi_event ev;
					ev.cmd.size = sizeof(gii_cmd_nodata_event);
					ev.cmd.type = evCommand;
					ev.cmd.code =GGICMD_ACKNOWLEDGE_SWITCH;
					ggiEventSend(ggivis, &ev);
					dontdraw = 1;
				}
			}
			break;
		case evExpose:
			dontdraw = 0;
			break;
		}
		nev--;
	}
}

void I_StartTic (void)
{
	if (ggivis) I_GetEvent();
}

void I_UpdateNoBlit (void)
{
}

void I_FinishUpdate (void)
{
	if (dontdraw) return;

#ifdef I_GGI_HERETIC
#ifndef NO_FILTERING
	/* Filters the first 32 palette-entry-color'd(gray-color) pixels
	   if wanted */
	if (lifilter && !bilifilter) {
		V_Filter_Screen_linear(SCREEN0_PTR);
	}
	if (bilifilter && !lifilter) {
		V_Filter_Screen_bilinear(SCREEN0_PTR);
	}
#endif
#endif

	if (!usedbuf) {
		int	i;

		if (havedbuf) {
			if (ggiResourceAcquire(dbuf1->resource,
					       GGI_ACTYPE_WRITE) != 0 ||
			    (doublebuffer ?
			     ggiResourceAcquire(dbuf2->resource,
						GGI_ACTYPE_WRITE) != 0
			     : 0)) {
				ggiPanic("Unable to acquire DirectBuffer!\n");
			}
			frameptr[0] = dbuf1->write;
			if (doublebuffer) {
				frameptr[1] = dbuf2->write;
			} else {
				frameptr[1] = frameptr[0];
			}
		}
		if (scale) {
			switch (pixelsize) {
			case 1:	if (havedbuf) {
				do_scale8(screenwidth, screenheight,
					  frameptr[curframe], SCREEN0_PTR);
			} else {
				uint8 *buf = SCREEN0_PTR;
				for (i=0; i < screenheight; i++) {
					do_scale8(screenwidth, 1, oneline,buf);
					ggiPutBox(ggivis, 0, i*2, realwidth,
						  2, oneline);
					buf += screenwidth;
				}
			}
			break;
			case 2: if (havedbuf) {
				do_scale16(screenwidth, screenheight,
					   frameptr[curframe], SCREEN0_PTR);
			} else {
				uint8 *buf = SCREEN0_PTR;
				for (i=0; i < screenheight; i++) {
					do_scale16(screenwidth, 1,
						   oneline, buf);
					ggiPutBox(ggivis, 0, i*2, realwidth,
						  2, oneline);
					buf += screenwidth;
				}
			}
			break;
			case 4: if (havedbuf) {
				do_scale32(screenwidth, screenheight,
					   frameptr[curframe], SCREEN0_PTR);
			} else {
				uint8 *buf = SCREEN0_PTR;
				for (i=0; i < screenheight; i++) {
					do_scale32(screenwidth, 1,
						   oneline, buf);
					ggiPutBox(ggivis, 0, i*2, realwidth,
						  2, oneline);
					buf += screenwidth;
				}
			}
			break;
			}
		} else if (palette) {
			switch (pixelsize) {
			case 1:	if (havedbuf) {
				do_copy8(screenwidth, screenheight,
					 frameptr[curframe], SCREEN0_PTR);
			} else {
				uint8 *buf = SCREEN0_PTR;
				for (i=0; i < screenheight; i++) {
					do_copy8(screenwidth, 1, oneline,buf);
					ggiPutBox(ggivis, 0, i, realwidth,
						  1, oneline);
					buf += screenwidth;
				}
			}
			break;
			case 2: if (havedbuf) {
				do_copy16(screenwidth, screenheight,
					  frameptr[curframe], SCREEN0_PTR);
			} else {
				uint8 *buf = SCREEN0_PTR;
				for (i=0; i < screenheight; i++) {
					do_copy16(screenwidth, 1,
						  oneline, buf);
					ggiPutBox(ggivis, 0, i, realwidth,
						  1, oneline);
					buf += screenwidth;
				}
			}
			break;
			case 4: if (havedbuf) {
				do_copy32(screenwidth, screenheight,
					  frameptr[curframe], SCREEN0_PTR);
			} else {
				uint8 *buf = SCREEN0_PTR;
				for (i=0; i < screenheight; i++) {
					do_copy32(screenwidth, 1,
						  oneline, buf);
					ggiPutBox(ggivis, 0, i, realwidth,
						  1, oneline);
					buf += screenwidth;
				}
			}
			break;
			}
		} else if (!modexrefresh) {
			/* faster, but ugly in ModeX modes */
			ggiPutBox(ggivis, 0, 0, screenwidth, screenheight,
				  SCREEN0_PTR);
		} else {
			/* slower, but nicer in ModeX modes */
			uint8 *buf = SCREEN0_PTR;
			for (i = 0; i < screenheight; i++) {
				ggiPutHLine(ggivis, 0, i, screenwidth, buf);
				buf += screenwidth;
			}
		}
		if (havedbuf) {
			ggiResourceRelease(dbuf1->resource);
			if (doublebuffer) {
				ggiResourceRelease(dbuf2->resource);
			}
		}

	}
#ifdef SHOW_FPS
	if (showfps) {
		struct timeval curtime;
		double diff;
		char str[64];
		
		totalframes++;
		gettimeofday(&curtime, NULL);
		diff = (curtime.tv_sec - starttime.tv_sec);
		diff += ((double)curtime.tv_usec - starttime.tv_usec)/1000000;
		if (diff != 0) {
			sprintf(str, "FPS: %.1f", totalframes/diff);
			ggiPuts(ggivis, 1, 1, str);
		}
	}
#endif
	if (doublebuffer) {
		ggiSetDisplayFrame(ggivis, curframe);
		curframe = !curframe;
		if (usedbuf) {
			SCREEN0_PTR = frameptr[curframe];
			/* Ouch, we need to recalculate the line offsets */
			R_InitBuffer(scaledviewwidth, viewheight);
		}
		ggiSetWriteFrame(ggivis, curframe);
	}
	
	ggiFlush(ggivis);
}

void I_SetPalette (byte* pal)
{
	int i;
	ggi_color col[256];
	byte *gamma = gammatable[usegamma];
	
	for (i = 0; i < 256; i++) {
		col[i].r = gamma[(*pal++)] << (GGI_COLOR_PRECISION-8);
		col[i].g = gamma[(*pal++)] << (GGI_COLOR_PRECISION-8);
		col[i].b = gamma[(*pal++)] << (GGI_COLOR_PRECISION-8);
	}
	if (palette) {
		ggiPackColors(ggivis, palette, col, 256);
	} else {
		ggiSetPalette(ggivis, 0, 256, col);
	}
}

void I_CheckRes()
{
}

void InitGraphLib(void)
{
}

void I_InitGraphics(void)
{
	ggi_mode mode;
	int noflicker = 0;

	fprintf(stderr, "I_InitGraphics: Init GGI-visual.\n");

	if (ggiInit() < 0) I_Error("Unable to init LibGGI!\n");
	if ((ggivis = ggiOpen(NULL)) == NULL) {
		I_Error("Unable to open default visual!\n");
	}
	ggiSetFlags(ggivis, GGIFLAG_ASYNC);
	ggiSetEventMask(ggivis, emKey | emPointer);

	modexrefresh = M_CheckParm("-modex") > 0;
	doublebuffer = M_CheckParm("-doublebuffer") > 0;
	scale = M_CheckParm("-scale") > 0;
	noflicker = M_CheckParm("-noflicker") > 0;

	realwidth = screenwidth;
	realheight = screenheight;
	if (scale) {
		realwidth *= 2;
		realheight *= 2;
	}

	if ((!doublebuffer || ggiCheckSimpleMode(ggivis, realwidth, realheight,
						 2, GT_8BIT, &mode) < 0) &&
	    (ggiCheckSimpleMode(ggivis, realwidth, realheight, GGI_AUTO,
				GT_8BIT, &mode) < 0) &&
#if defined(I_GGI_HERETIC)
/* Heretic still only works reliably in 320x200 (640x400 if scaled) */
	    (ggiCheckSimpleMode(ggivis, realwidth, realheight, GGI_AUTO,
				GT_AUTO, &mode) < 0 )) {
		I_Error("Can't set %ix%i mode\n",
			realwidth, realheight);
#elif defined(I_GGI_DOOM)
		(ggiCheckSimpleMode(ggivis, realwidth, realheight, GGI_AUTO,
				    GT_AUTO, &mode) < 0) &&
			(mode.visible.x > MAXSCREENWIDTH ||
			 mode.visible.y > MAXSCREENHEIGHT)) {
					I_Error("Can't find a suitable mode\n",
						realwidth, realheight);
#endif
				}
	if (ggiSetMode(ggivis, &mode) < 0) {
		I_Error("LibGGI can't set any modes at all?!\n");
	}
    
	realwidth  = mode.visible.x;
	realheight = mode.visible.y;
	if (scale) {
		screenwidth  = realwidth / 2;
		screenheight = realheight / 2; 
	} else {
		screenwidth  = realwidth;
		screenheight = realheight;
	}
	
	pixelsize = (GT_SIZE(mode.graphtype)+7) / 8;
	if (mode.graphtype != GT_8BIT) {
		if ((palette = malloc(pixelsize*256)) == NULL) {
			I_Error("Unable to allocate memory?!\n");
		}
	}

	
	usedbuf = havedbuf = 0;
	stride = realwidth*pixelsize;
	if ((dbuf1 = ggiDBGetBuffer(ggivis, 0)) != NULL &&
	    (dbuf1->type & GGI_DB_SIMPLE_PLB) &&
	    (doublebuffer ? ((dbuf2 = ggiDBGetBuffer(ggivis, 1)) != NULL &&
			     (dbuf2->type & GGI_DB_SIMPLE_PLB)) : 1)) {
		havedbuf = 1;
		stride = dbuf1->buffer.plb.stride;
		fprintf(stderr, "I_InitGraphics: Have DirectBuffer visual\n");
		if (!noflicker && !scale && !palette &&
		    stride == pixelsize*realwidth &&
		    !ggiResourceMustAcquire(dbuf1->resource) &&
		    (doublebuffer ? !ggiResourceMustAcquire(dbuf2->resource)
		     : 1)) {
			usedbuf = 1;
			frameptr[0] = dbuf1->write;
			if (doublebuffer) {
				frameptr[1] = dbuf2->write;
			} else {
				frameptr[1] = frameptr[0];
			}
			SCREEN0_PTR = frameptr[0];
			fprintf(stderr,
				"I_InitGraphics: Using DirectBuffer with");
			if (doublebuffer)	{
				fprintf(stderr, " doublebuffering\n");
			} else {
				fprintf(stderr, " singlebuffering\n");
			}
		}
	}
	if (!usedbuf) {
		ggi_event ev;
		ev.cmd.size = sizeof(gii_cmd_nodata_event);
		ev.cmd.type = evCommand;
		ev.cmd.code = GGICMD_NOHALT_ON_UNMAP;
		ggiEventSend(ggivis, &ev);

		ggiAddEventMask(ggivis, emCommand | emExpose);

		if ((SCREEN0_PTR = malloc(screenwidth*screenheight)) == NULL) {
			I_Error("Unable to allocate memory?!\n");
		}
		if (!havedbuf && (scale || palette)) {
			int linesize = pixelsize*realwidth;
			if (scale) linesize *= 4;
			if ((oneline = malloc(linesize)) == NULL) {
				I_Error("Unable to allocate memory?!\n");
			}
		}
		fprintf(stderr,
			"I_InitGraphics: Drawing into offscreen memory\n");
	}
	/* We will start drawing to frame 0, and start displaying frame 1 */
	if (doublebuffer)	{
		ggiSetWriteFrame(ggivis, 0);
		ggiSetDisplayFrame(ggivis, 1);
	}

	curframe = 0;
}
