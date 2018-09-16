#include <unistd.h>

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/XShm.h>
/*
 * Had to dig up XShm.c for this one.
 * It is in the libXext, but not in the XFree86 headers.
 */
#ifdef NEED_SHMGETEVENTBASE
int XShmGetEventBase( Display* dpy );
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomdef.h"

#define SHOW_FPS 
 
#ifdef SHOW_FPS 
static struct timeval   starttime; 
static long             totalframes; 
static int              showfps = 0; 

static void reset_framecounter(void) 
{ 
  totalframes = 0; 
  gettimeofday(&starttime, NULL); 
} 
#endif

#define POINTER_WARP_COUNTDOWN	1

static Display       *X_display=0;
static Window        X_mainWindow;
static Colormap      X_cmap;
static Visual        *X_visual;
static GC            X_gc;
static XEvent        X_event;
static int           X_screen;
static XVisualInfo   X_visualinfo;
static XImage        *image;
static int           X_width;
static int           X_height;
static int           true_color=FALSE;
static Screen        *screen_intern;
static XColor        colors[256];
static unsigned long *xpixel;

/* MIT SHared Memory extension. */
static boolean       doShm;

static XShmSegmentInfo X_shminfo;
static int             X_shmeventtype;
static int             XShm_is_attached=FALSE;


/*
 * Fake mouse handling.
 * This cannot work properly w/o DGA.
 * Needs an invisible mouse cursor at least.
 */
static boolean  grabMouse;
static int      doPointerWarp = POINTER_WARP_COUNTDOWN;

/*
 * Blocky mode,
 * replace each 320x200 pixel with multiply*multiply pixels.
 * According to Dave Taylor, it still is a bonehead thing
 * to use ....
 */
static int	multiply=1;

/*
 * pointer to palette update function (different for
 * true- and pseudocolor)
 */
static void (*up_palette)(Colormap,byte *);

/* truecolor helper routines */
static int init_truec_pals(void);
static int free_truec_pals(void);
static void fill_pal_cache(int,Colormap,byte *);
static int get_pal_usage(void);

static int oldgamma;

/*#define PALETTE_DEBUG*/
#define PALETTE_INFO       /* display # of used cache slots at termination */
#define MAX_PAL_CACHE 25   /* number of palettes to cache */
#define INI_LRU 1000
#define MIN_LRU 10

/* layout of entry in truecolor palette cache */
static struct paldef {
  caddr_t id;               /* contains pointer / id of palette */
  unsigned long *xpix_ptr;  /* pointer to matching xpixel table */
  unsigned int lru;
} *pal_cache;


static inline void clr_pal_cache(int index,Colormap cmap)
{
  int j;
  
  for (j=0; j<256; j++) {
    XFreeColors(X_display,cmap,pal_cache[index].xpix_ptr+j,1,0);
  }
}

/*
 *  Translates the key currently in X_event
 */

int xlatekey(void)
{
  
    int rc;
    
    switch(rc = XKeycodeToKeysym(X_display, X_event.xkey.keycode, 0))
      {
      case XK_Delete:   rc = KEY_DELETE;        break; 
      case XK_Insert:   rc = KEY_INSERT;        break; 
      case XK_Page_Up:  rc = KEY_PAGEUP;        break; 
      case XK_Page_Down:rc = KEY_PAGEDOWN;      break; 
      case XK_Home:     rc = KEY_HOME;          break; 
      case XK_End:      rc = KEY_END;           break;
      case XK_Left:	rc = KEY_LEFTARROW;	break;
      case XK_Right:	rc = KEY_RIGHTARROW;	break;
      case XK_Down:	rc = KEY_DOWNARROW;	break;
      case XK_Up:	rc = KEY_UPARROW;	break;
      case XK_Escape:	rc = KEY_ESCAPE;	break;
      case XK_Return:	rc = KEY_ENTER;		break;
      case XK_Tab:	rc = KEY_TAB;		break;
      case XK_F1:	rc = KEY_F1;		break;
      case XK_F2:	rc = KEY_F2;		break;
      case XK_F3:	rc = KEY_F3;		break;
      case XK_F4:	rc = KEY_F4;		break;
      case XK_F5:	rc = KEY_F5;		break;
      case XK_F6:	rc = KEY_F6;		break;
      case XK_F7:	rc = KEY_F7;		break;
      case XK_F8:	rc = KEY_F8;		break;
      case XK_F9:	rc = KEY_F9;		break;
      case XK_F10:	rc = KEY_F10;		break;
      case XK_F11:	rc = KEY_F11;		break;
      case XK_F12:	rc = KEY_F12;		break;

      case XK_BackSpace:rc = KEY_BACKSPACE;     break;
      case XK_Pause:	rc = KEY_PAUSE;		break;

      case XK_KP_Add:
      case XK_plus:     
	/*   rc = KEY_PLUS; break; !TODO: define in doomdef.h TODO!   */
      case XK_KP_Equal:
      case XK_equal:	rc = KEY_EQUALS;	break;
	
      case XK_KP_Subtract:
      case XK_minus:	rc = KEY_MINUS;		break;
	
      case XK_Shift_L:
      case XK_Shift_R:
	rc = KEY_RSHIFT;
	break;

      case XK_Control_L:
      case XK_Control_R:
	rc = KEY_RCTRL;
	break;
	
      case XK_Alt_L:
      case XK_Meta_L:
      case XK_Alt_R:
      case XK_Meta_R:
	rc = KEY_RALT;
	break;

      default:
	if (rc >= XK_space && rc <= XK_asciitilde)
	  rc = rc - XK_space + ' ';
	if (rc >= 'A' && rc <= 'Z')
	  rc = rc - 'A' + 'a';
	break;
    }
    
    return rc;
}

void I_ShutdownGraphics(void)
{
  if (true_color) {
#ifdef PALETTE_INFO
    fprintf(stderr,"TrueColor palette cache: %d/%d entries used.\n",
	    get_pal_usage(),MAX_PAL_CACHE);
#endif
    free_truec_pals();
    sleep(2);
  }
  
  if (XShm_is_attached) {
    /* Detach from X server */
    if (!XShmDetach(X_display, &X_shminfo))
      I_Error("XShmDetach() failed in I_ShutdownGraphics()");
    
    /* Release shared memory. */
    shmdt(X_shminfo.shmaddr);
    shmctl(X_shminfo.shmid, IPC_RMID, 0);
  }

  /* Paranoia. Yes: needed! */
  if (image)
    image->data = NULL;
}


/*
 * I_StartFrame
 */
void I_StartFrame (void)
{
  /* er? */
}

static int      lastmousex = 0;
static int      lastmousey = 0;
static boolean  mousemoved = false;
static boolean  shmFinished;

void I_GetEvent(void)
{
  
  event_t event;
  
  /* put event-grabbing stuff in here */
  XNextEvent(X_display, &X_event);
  switch (X_event.type)
    {
    case KeyPress:
      event.type = ev_keydown;
      event.data1 = xlatekey();
#ifdef SHOW_FPS 
      if (event.data1 == KEY_BACKSPACE && 
	  gamestate == GS_LEVEL) { 
	/* Toggle and reset the FPS counter */ 
	showfps = !showfps; 
	reset_framecounter(); 
      } 
#endif
      D_PostEvent(&event);
      /* fprintf(stderr, "k"); */
      break;
    case KeyRelease:
      event.type = ev_keyup;
      event.data1 = xlatekey();
      D_PostEvent(&event);
      /* fprintf(stderr, "ku"); */
      break;
    case ButtonPress:
      event.type = ev_mouse;
      event.data1 =
	(X_event.xbutton.state & Button1Mask)
	| (X_event.xbutton.state & Button2Mask ? 2 : 0)
	| (X_event.xbutton.state & Button3Mask ? 4 : 0)
	| (X_event.xbutton.button == Button1)
	| (X_event.xbutton.button == Button2 ? 2 : 0)
	| (X_event.xbutton.button == Button3 ? 4 : 0);
      event.data2 = event.data3 = 0;
      D_PostEvent(&event);
      /* fprintf(stderr, "b"); */
      break;
    case ButtonRelease:
      event.type = ev_mouse;
      event.data1 =
	(X_event.xbutton.state & Button1Mask)
	| (X_event.xbutton.state & Button2Mask ? 2 : 0)
	| (X_event.xbutton.state & Button3Mask ? 4 : 0);
      /* suggest parentheses around arithmetic in operand of | */
      event.data1 =
	event.data1
	^ (X_event.xbutton.button == Button1 ? 1 : 0)
	^ (X_event.xbutton.button == Button2 ? 2 : 0)
	^ (X_event.xbutton.button == Button3 ? 4 : 0);
      event.data2 = event.data3 = 0;
      D_PostEvent(&event);
      /* fprintf(stderr, "bu"); */
      break;
    case MotionNotify:
      event.type = ev_mouse;
      event.data1 =
	(X_event.xmotion.state & Button1Mask)
	| (X_event.xmotion.state & Button2Mask ? 2 : 0)
	| (X_event.xmotion.state & Button3Mask ? 4 : 0);
      event.data2 = (X_event.xmotion.x - lastmousex) << 2;
      event.data3 = (lastmousey - X_event.xmotion.y) << 2;
      
      if (event.data2 || event.data3)
	{
	  lastmousex = X_event.xmotion.x;
	  lastmousey = X_event.xmotion.y;
	  if (X_event.xmotion.x != X_width/2 &&
	      X_event.xmotion.y != X_height/2)
	    {
	      D_PostEvent(&event);
	      /* fprintf(stderr, "m"); */
	      mousemoved = false;
	    } else
	      {
		mousemoved = true;
	      }
	}
      break;
      
    case Expose:
    case ConfigureNotify:
      break;
      
    default:
      if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
      break;
    }  
}

static Cursor createnullcursor(Display* display,Window root)
{
  Pixmap cursormask;
  XGCValues xgc;
  GC gc;
  XColor dummycolour;
  Cursor cursor;
  
  cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
  xgc.function = GXclear;
  gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
  XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
  dummycolour.pixel = 0;
  dummycolour.red = 0;
  dummycolour.flags = 04;
  cursor = XCreatePixmapCursor(display, cursormask, cursormask,
			       &dummycolour,&dummycolour, 0,0);
  XFreePixmap(display,cursormask);
  XFreeGC(display,gc);
  return cursor;
}

/*
 * I_StartTic
 */
void I_StartTic (void)
{
  if (!X_display)
    return;
  
  while (XPending(X_display))
    I_GetEvent();
  
  /*
   * Warp the pointer back to the middle of the window
   *  or it will wander off - that is, the game will
   *  loose input focus within X11.
   */
  if (grabMouse)
    {
      if (!--doPointerWarp)
	{
	  XWarpPointer( X_display,
			None,
			X_mainWindow,
			0, 0,
			0, 0,
			X_width/2, X_height/2);
	  
	  doPointerWarp = POINTER_WARP_COUNTDOWN;
	}
    }
  
  mousemoved = false;
}


/*
 * I_FinishUpdate
 */
void I_FinishUpdate (void)
{
  static int    lasttic=0;
  int           tics,i,l,l2,c,cc,ll;
  unsigned char p;
  long          color;
  boolean       devparm=1; /* You can change this to 1 for little dots on the screen ! */
  
  /* draws little dots on the bottom of the screen */
  if (devparm) {
    i = I_GetTime();
    tics = i - lasttic;
    lasttic = i;
    if (tics > 20) tics = 20;
    
    for (i=0 ; i<tics*2 ; i+=2)
      screen[ (screenheight-1)*screenwidth + i] = 0xff;
    for ( ; i<20*2 ; i+=2)
      screen[ (screenheight-1)*screenwidth + i] = 0x0;
  }
 
  /* Filters the first 32 palette-entry-color'd(gray-color) pixels if wanted */
  if (lifilter && !bilifilter)
    V_Filter_Screen_linear(screen);
  if (bilifilter && !lifilter)
    V_Filter_Screen_bilinear(screen);

  if (true_color) {
    if (multiply == 1) {
      for (l=0,l2=0; l<screenheight; l++, l2+=screenwidth) {
	for (c=0; c<screenwidth; c++) {
	  p=screen[l2+c];
	  color=xpixel[p];
	  XPutPixel(image,c,l,color);
	}
      }
    }
    else if (multiply == 2) {
      for (l=0,l2=0; l<screenheight; l++, l2+=screenwidth) {
	for (c=0; c<screenwidth; c++) {
	  p=screen[l2+c];
	  color=xpixel[p];
	  XPutPixel(image,c+c,  l+l,  color);
	  XPutPixel(image,c+c+1,l+l,  color);
	  XPutPixel(image,c+c,  l+l+1,color);
	  XPutPixel(image,c+c+1,l+l+1,color);
	}
      }
    }
    else if (multiply == 3) {
      for (l=0,l2=0; l<screenheight; l++, l2+=screenwidth) {
	for (c=0; c<screenwidth; c++) {
	  p=screen[l2+c];
	  color=xpixel[p];
	  cc=c+c+c; ll=l+l+l;
	  XPutPixel(image,cc,  ll,  color);
	  XPutPixel(image,cc+1,ll,  color);
	  XPutPixel(image,cc+2,ll,  color);
	  XPutPixel(image,cc,  ll+1,color);
	  XPutPixel(image,cc+1,ll+1,color);
	  XPutPixel(image,cc+2,ll+1,color);
	  XPutPixel(image,cc,  ll+2,color);
	  XPutPixel(image,cc+1,ll+2,color);
	  XPutPixel(image,cc+2,ll+2,color);
	}
      }
    }
    else {  /* multiply == 4 */
      for (l=0,l2=0; l<screenheight; l++, l2+=screenwidth) {
	for (c=0; c<screenwidth; c++) {
	  p=screen[l2+c];
	  color=xpixel[p];
	  cc=c<<2; ll=l<<2;
	  XPutPixel(image,cc,  ll,  color);
	  XPutPixel(image,cc+1,ll,  color);
	  XPutPixel(image,cc+2,ll,  color);
	  XPutPixel(image,cc+3,ll,  color);
	  XPutPixel(image,cc,  ll+1,color);
	  XPutPixel(image,cc+1,ll+1,color);
	  XPutPixel(image,cc+2,ll+1,color);
	  XPutPixel(image,cc+3,ll+1,color);
	  XPutPixel(image,cc,  ll+2,color);
	  XPutPixel(image,cc+1,ll+2,color);
	  XPutPixel(image,cc+2,ll+2,color);
	  XPutPixel(image,cc+3,ll+2,color);
	  XPutPixel(image,cc,  ll+3,color);
	  XPutPixel(image,cc+1,ll+3,color);
	  XPutPixel(image,cc+2,ll+3,color);
	  XPutPixel(image,cc+3,ll+3,color);
	}
      }
    }
  }
  else {  /* not truecolor */
    /* scales the screen size before blitting it */
    if (multiply == 2) {
      unsigned int *olineptrs[2];
      unsigned int *ilineptr;
      int x,y;
      unsigned int twoopixels;
      unsigned int twomoreopixels;
      unsigned int fouripixels;
      
      ilineptr = (unsigned int *) (screen);
      for (i=0 ; i<2 ; i++)
	olineptrs[i] = (unsigned int *) &image->data[i*X_width];
      
      y = screenheight;
      while (y--) {
	x = screenwidth;
	do {
	  fouripixels = *ilineptr++;
	  twoopixels = (fouripixels & 0xff000000)
	    | ((fouripixels>>8) & 0xffff00)
	    | ((fouripixels>>16) & 0xff);
	  twomoreopixels = ((fouripixels<<16) & 0xff000000)
	    | ((fouripixels<<8) & 0xffff00)
	    | (fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
	  *olineptrs[0]++ = twoopixels;
	  *olineptrs[1]++ = twoopixels;
	  *olineptrs[0]++ = twomoreopixels;
	  *olineptrs[1]++ = twomoreopixels;
#else
	  *olineptrs[0]++ = twomoreopixels;
	  *olineptrs[1]++ = twomoreopixels;
	  *olineptrs[0]++ = twoopixels;
	  *olineptrs[1]++ = twoopixels;
#endif
	} while (x-=4);
	olineptrs[0] += X_width/4;
	olineptrs[1] += X_width/4;
      }
    }
    else if (multiply == 3) {
      unsigned int *olineptrs[3];
      unsigned int *ilineptr;
      int x,y;
      unsigned int fouropixels[3];
      unsigned int fouripixels;
      
      ilineptr = (unsigned int *) (screen);
      for (i=0 ; i<3 ; i++)
	olineptrs[i] = (unsigned int *) &image->data[i*X_width];
      
      y = screenheight;
      while (y--) {
	x = screenwidth;
	do {
	  fouripixels = *ilineptr++;
	  fouropixels[0] = (fouripixels & 0xff000000)
	    | ((fouripixels>>8) & 0xff0000)
	    | ((fouripixels>>16) & 0xffff);
	  fouropixels[1] = ((fouripixels<<8) & 0xff000000)
	    | (fouripixels & 0xffff00)
	    | ((fouripixels>>8) & 0xff);
	  fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
	    | ((fouripixels<<8) & 0xff00)
	    | (fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
	  *olineptrs[0]++ = fouropixels[0];
	  *olineptrs[1]++ = fouropixels[0];
	  *olineptrs[2]++ = fouropixels[0];
	  *olineptrs[0]++ = fouropixels[1];
	  *olineptrs[1]++ = fouropixels[1];
	  *olineptrs[2]++ = fouropixels[1];
	  *olineptrs[0]++ = fouropixels[2];
	  *olineptrs[1]++ = fouropixels[2];
	  *olineptrs[2]++ = fouropixels[2];
#else
	  *olineptrs[0]++ = fouropixels[2];
	  *olineptrs[1]++ = fouropixels[2];
	  *olineptrs[2]++ = fouropixels[2];
	  *olineptrs[0]++ = fouropixels[1];
	  *olineptrs[1]++ = fouropixels[1];
	  *olineptrs[2]++ = fouropixels[1];
	  *olineptrs[0]++ = fouropixels[0];
	  *olineptrs[1]++ = fouropixels[0];
	  *olineptrs[2]++ = fouropixels[0];
#endif
	} while (x-=4);
	olineptrs[0] += 2*X_width/4;
	olineptrs[1] += 2*X_width/4;
	olineptrs[2] += 2*X_width/4;
      }
    }
    else if (multiply == 4) {
      /* Broken. Gotta fix this some day. */
      void Expand4(unsigned *, double *);
      Expand4 ((unsigned *)(screen), (double *) (image->data));
    }
  } /* if not truecolor */
  
  if (doShm) {
    if (!XShmPutImage(X_display,
		      X_mainWindow,
		      X_gc,
		      image,
		      0, 0,
		      0, 0,
		      X_width,X_height,
		      True))
      I_Error("XShmPutImage() failed\n");
    
    /* wait for it to finish and processes all input events */
    shmFinished = false;
    do {
      I_GetEvent();
    } while (!shmFinished);
  }
  else {
    /* draw the image */
    XPutImage(X_display,
	      X_mainWindow,
	      X_gc,
	      image,
	      0, 0,
	      0, 0,
	      X_width,X_height);
    
    /* sync up with server */
    XSync(X_display, False);
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
      XDrawString(X_display, X_mainWindow, X_gc, 0, 16, str, 
		  strlen(str)); 
    } 
  } 
#endif 
}


/*
 * Palette stuff.
 *
 * PseudoColor version
 */
static void UploadNewPalette(Colormap cmap, byte *palette)
{
    register int   i;
    register int   c;
    static boolean firstcall = TRUE;
    
    if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8) {
      /* initialize the colormap */
      if (firstcall) {
	firstcall = FALSE;
	for (i=0 ; i<256 ; i++) {
	  colors[i].pixel = i;
	  colors[i].flags = DoRed|DoGreen|DoBlue;
	}
      }
      
      /* set the X colormap entries */
      for (i=0 ; i<256 ; i++)  {
	c = gammatable[usegamma][*palette++];
	colors[i].red = (c<<8) + c;
	c = gammatable[usegamma][*palette++];
	colors[i].green = (c<<8) + c;
	c = gammatable[usegamma][*palette++];
	colors[i].blue = (c<<8) + c;
      }
      
      /* store the colors to the current colormap */
      XStoreColors(X_display, cmap, colors, 256);
    }
    else {
      fprintf(stderr,"UploadNewPalette: unknown visual type!\n");
    }
}

/*
 * Truecolor "palette" update
 */
static void UploadNewTruePal(Colormap cmap, byte *palette)
{
  int least,lru,i,found=FALSE;
  
  /* use the palette pointer as palette id, i.e. a palette
   * with another pointer value is assumed to be a different
   * palette, and, more important, if the pointer is equal,
   * the palette is assumed to be the same...
   */
  
#ifdef PALETTE_DEBUG
  fprintf(stderr,"UploadNewTruePal: entered with %p -- gamma %d\n",palette,usegamma);
#endif
  
  if (usegamma != oldgamma) {  /* if gamma value changed since last invokation */
#ifdef PALETTE_DEBUG
    fprintf(stderr,"\tnew gamma value -- discarding cache\n");
#endif
    oldgamma=usegamma;
    for (i=0; i<MAX_PAL_CACHE; i++) {  /* discard old palette cache */
      if (pal_cache[i].id) clr_pal_cache(i,cmap);
      pal_cache[i].id=0;  /* mark entry as empty */
    }
  }
  
  /* search ptr in table */
  for (i=0; i<MAX_PAL_CACHE; i++) {
    if (pal_cache[i].id == (caddr_t)palette) {
#ifdef PALETTE_DEBUG
      fprintf(stderr,"\tfound palette, index %d\n",i);
#endif
      xpixel=pal_cache[i].xpix_ptr;
      pal_cache[i].lru=INI_LRU;
      found=TRUE;
    }
    else {
      if (pal_cache[i].lru > MIN_LRU) pal_cache[i].lru--;
    }
  }
  if (found) return;
#ifdef PALETTE_DEBUG
  fprintf (stderr,"\tNOT FOUND\n");
#endif
  
  /* palette not yet in my table -- search empty entry */
  for (i=0, found=FALSE; i<MAX_PAL_CACHE; i++) {
    if (! pal_cache[i].id) {
      found=TRUE;
      break;
    }
  }
  if (found) {
#ifdef PALETTE_DEBUG
    fprintf(stderr,"\tfound empty: index %d\n",i);
#endif
    fill_pal_cache(i,cmap,palette);
    return;
  }
#ifdef PALETTE_DEBUG
  fprintf(stderr,"\tno empty entry found \n");
#endif
  
  /* no empty entry -- search a least recently used entry */
  for (least=-1,i=0,lru=INI_LRU; i<MAX_PAL_CACHE; i++) {
    if (pal_cache[i].lru < (size_t)lru) {
      lru=pal_cache[i].lru;
      least=i;
    }
  }
  if (least == -1) {
    fprintf(stderr,"UploadNewTruePal: palette cache inconsistency!\n");
    return;
  }
#ifdef PALETTE_DEBUG
  fprintf(stderr,"\treusing entry %d\n",least);
#endif
  clr_pal_cache(least,cmap);   /* free old entries */
  fill_pal_cache(least,cmap,palette); /* alloc new entries */
  return;
}

static void fill_pal_cache(int index,Colormap cmap,byte *palette)
{
  unsigned int i,c;
  XColor color;
  void *dummy;
  
  if (!pal_cache[index].xpix_ptr) { /* memory for palette not yet allocated */
      {
	  dummy=malloc(256 * sizeof(long));
	  assert(dummy);
	  if (! (pal_cache[index].xpix_ptr=dummy))
	      {
		  fprintf(stderr,"fill_pal_cache: failed to allocate palette buffer\n");
		  return;
	      }
      }
  }
  
  xpixel=pal_cache[index].xpix_ptr;
  pal_cache[index].id=(caddr_t)palette;  /* set id */
  
  for (i=0; i<256; i++) {
    color.pixel = 0;
    color.flags = DoRed|DoGreen|DoBlue;
    c = gammatable[usegamma][*palette++];
    color.red = c << 8;
    c = gammatable[usegamma][*palette++];
    color.green = c << 8;
    c = gammatable[usegamma][*palette++];
    color.blue = c << 8;
    
    if (XAllocColor(X_display,cmap,&color)) {
      xpixel[i]=color.pixel;
    }
    else {
      fprintf(stderr,"fill_pal_cache: cannot alloc color %d\n",i);
    }
  }
}

static int get_pal_usage(void)
{
  int i,count=0;
  
  if (pal_cache)
    for (i=0; i<MAX_PAL_CACHE; i++)
      if (pal_cache[i].id)
	count++;
  return(count);
}

static int init_truec_pals(void)
{
  pal_cache=malloc(MAX_PAL_CACHE * sizeof(struct paldef));
  assert(pal_cache);
  if (!pal_cache) return(FALSE);
  memset(pal_cache,0,MAX_PAL_CACHE * sizeof(struct paldef));
  return(TRUE);
}

static int free_truec_pals(void)
{
  int i;
  
  if (pal_cache) {
    for (i=0; i<MAX_PAL_CACHE; i++) {
      if (pal_cache[i].xpix_ptr) {
	clr_pal_cache(i,X_cmap);
	free(pal_cache[i].xpix_ptr);
      }
    }
    free(pal_cache);
  }
  return(TRUE);
}

/*
 * I_SetPalette
 */
void I_SetPalette (byte* palette)
{
  if (up_palette)
    (up_palette)(X_cmap, palette);
  else
    fprintf(stderr,"i_x11: up_palette called before initialization!!!\n");
}


/*
 * This function is probably redundant,
 *  if XShmDetach works properly.
 * ddt never detached the XShm memory,
 *  thus there might have been stale
 *  handles accumulating.
 */
void grabsharedmemory(size_t size)
{
  
  int			key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds	shminfo;
  int			minsize = screenwidth * screenheight; /* 320*200; */
  int			id;
  int			rc;
  /* UNUSED int done=0; */
  int			pollution=5;
  
  /* try to use what was here before */
  do
    {
      id = shmget((key_t) key, minsize, 0777); /* just get the id */
      if (id != -1)
	{
	  rc=shmctl(id, IPC_STAT, &shminfo);   /* get stats on it */
	  if (!rc)
	    {
	      if (shminfo.shm_nattch)
		{
		  fprintf(stderr, "User %d appears to be running "
			  "Heretic.  Is that wise?\n", shminfo.shm_cpid);
		  key++;
		}
	      else
		{
		  if (getuid() == shminfo.shm_perm.cuid)
		    {
		      rc = shmctl(id, IPC_RMID, 0);
		      if (!rc)
			fprintf(stderr,
				"Was able to kill my old shared memory\n");
		      else
			I_Error("Was NOT able to kill my old shared memory");
		      
		      id = shmget((key_t)key, size, IPC_CREAT|0777);
		      if (id==-1)
			I_Error("Could not get shared memory");
		      
		      rc=shmctl(id, IPC_STAT, &shminfo);
		      
		      break;
		      
		    }
		  if (size >= shminfo.shm_segsz)
		    {
		      fprintf(stderr,
			      "will use %d's stale shared memory\n",
			      shminfo.shm_cpid);
		      break;
		    }
		  else
		    {
		      fprintf(stderr,
			      "warning: can't use stale "
			      "shared memory belonging to id %d, "
			      "key=0x%x\n",
			      shminfo.shm_cpid, key);
		      key++;
		    }
		}
	    }
	  else
	    {
	      I_Error("could not get stats on key=%d", key);
	    }
	}
      else
	{
	  id = shmget((key_t)key, size, IPC_CREAT|0777);
	  if (id==-1)
	    {
	      /* extern int errno; */
	      fprintf(stderr, "errno=%d\n", errno);
	      I_Error("Could not get any shared memory");
	    }
	  break;
	}
    } while (--pollution);
  
  if (!pollution)
    {
      I_Error("Sorry, system too polluted with stale "
	      "shared memory segments.\n");
    }
  
  X_shminfo.shmid = id;
  
  /* attach to the shared memory segment */
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);
  
  fprintf(stderr, "shared memory id=%d, addr=0x%lx\n", id,
	  (unsigned long) (image->data));
}

void InitGraphLib(void)
{
  /* not needed, dummy for X11 */
}

void I_InitGraphics(void)
{
  char*		displayname;
  char*		d;
  int		n;
  int		pnum;
  int		x=0;
  int		y=0;
  char          *dummy_data;
  
  
  /* warning: char format, different type arg */
  /*
   * char		        xsign=' '; 
   * char		        ysign=' ';  
   */
  int			oktodraw;
  unsigned long	        attribmask;
  XSetWindowAttributes  attribs;
  XGCValues		xgcvalues;
  XSizeHints            hints = {flags : 0};
  int			valuemask;
  static int		firsttime=1;
  
    if (!firsttime)
      return;
    firsttime = 0;
    
    signal(SIGINT, (void (*)(int)) I_Quit);
    
    if (M_CheckParm("-2"))
      multiply = 2;
    
    if (M_CheckParm("-3"))
      multiply = 3;
    
    if (M_CheckParm("-4"))
      multiply = 4;
    
    if (M_CheckParm("-mode") && multiply != 1) {
      multiply=1;  /* multiply only works in 320x200 mode yet */
      fprintf(stderr,"Screen stretching disabled due to -mode switch\n");
    }
    
    X_width = screenwidth * multiply;
    X_height = screenheight * multiply;
    
    /* check for command-line display name */
    if ((pnum = M_CheckParm("-display")))	
	displayname = myargv[pnum+1];
    else
	displayname = 0;
    
    /* check if the user wants to grab the mouse (quite unnice) */
    grabMouse = !!M_CheckParm("-grabmouse");        
    
    /* open the display */
    X_display = XOpenDisplay(displayname);
    if (!X_display)
      {
	  if (displayname)
	      I_Error("Could not open display [%s]", displayname);
	  else
	      I_Error("Could not open display (DISPLAY=[%s])", getenv("DISPLAY"));
      }
    
    /* use the default visual */
    screen_intern = DefaultScreenOfDisplay(X_display);
    X_screen = DefaultScreen(X_display);
    if (!XMatchVisualInfo(X_display,X_screen,8,PseudoColor,&X_visualinfo)) {
      true_color=TRUE;  /* OK, be slower, but try to work.... */
      if (XMatchVisualInfo(X_display,X_screen,8,TrueColor,&X_visualinfo)) {
	fprintf(stderr,"I_InitGraphics: using 8bit truecolor visual\n");
      }
      else if (XMatchVisualInfo(X_display,X_screen,16,TrueColor,&X_visualinfo)) {
	fprintf(stderr,"I_InitGraphics: using 16bit truecolor visual\n");
      }
      else if (XMatchVisualInfo(X_display,X_screen,15,TrueColor,&X_visualinfo)) {
	fprintf(stderr,"I_InitGraphics: using 15bit truecolor visual\n");
      }
      else if (XMatchVisualInfo(X_display,X_screen,24,TrueColor,&X_visualinfo)) {
	fprintf(stderr,"I_InitGraphics: using 24bit truecolor visual\n");
      }
      else if (XMatchVisualInfo(X_display,X_screen,32,TrueColor,&X_visualinfo)) {
	fprintf(stderr,"I_InitGraphics: using 32bit truecolor visual\n");
      }
      else {
	I_Error("unsupported display type (not 8, 16, 24 or 32bit colors)");
      }
    }
    else {
      fprintf(stderr,"I_InitGraphics: using 8bit pseudocolor visual, good!\n");
    }
    X_visual = X_visualinfo.visual;
    X_visual->red_mask=0;
    X_visual->blue_mask=0;
    X_visual->green_mask=0;
    
    if (!true_color) {
      up_palette=UploadNewPalette;  /* set update-palette function */
    }
    else {
      up_palette=UploadNewTruePal;
      oldgamma=usegamma;
      if (!init_truec_pals()) {
	I_Error("Truecolor mode: cannot allocate memory for palette cache");
      }
    }
    
    /* check for the MITSHM extension */
    if (!M_CheckParm("-noshm")) 
      doShm = XShmQueryExtension(X_display);
    else {
      doShm =FALSE;
      fprintf(stderr,"I_InitGraphics: SHM disabled via user request!\n");
    }
    
    /* even if it's available, make sure it's a local connection */
    if (doShm)
      {
	if (!displayname) displayname = (char *) getenv("DISPLAY");
	if (displayname)
	  {
	    d = displayname;
	    while (*d && (*d != ':')) d++;
	    if (*d) *d = 0;
	    if (!(!strcasecmp(displayname, "unix") || !*displayname)) doShm = false;
	  }
      }
    
    if (doShm)
      fprintf(stderr, "Using MITSHM extension\n");
    else
      fprintf(stderr, "MITSHM extension not available, disabled or remote display\n");
    
    /* create the colormap */
    if (!true_color)
      X_cmap = XCreateColormap(X_display,RootWindow(X_display,X_screen),
			       X_visual,AllocAll);
    else
      X_cmap = DefaultColormapOfScreen(screen_intern);
    
    /* setup attributes for main window */
    attribmask = CWEventMask | CWColormap | CWBorderPixel;
    attribs.event_mask = KeyPressMask
      | KeyReleaseMask
      /* | PointerMotionMask | ButtonPressMask | ButtonReleaseMask */
      | ExposureMask;
    
    attribs.colormap = X_cmap;
    attribs.border_pixel = 0;

    /* set up position and size hints */
    hints.width = X_width;
    hints.height = X_height;
    hints.min_width = X_width;
    hints.max_width = X_width;
    hints.min_height = X_height;
    hints.max_height = X_height;
    hints.flags |= USSize | PMinSize | PMaxSize;

    /* check for command-line geometry */
    if ((pnum = M_CheckParm("-geometry")))
	{
	    /* not used, really */
	    unsigned int w, h;
	    n = XParseGeometry(myargv[pnum + 1], &x, &y, &w, &h);
	    
	    if (!(n & AllValues))
		I_Error("bad -geometry parameter");

	    if ((n & XValue) || (n & YValue)) { } /* Whut? Xp */
	    hints.flags |= USPosition;
	    
	    if (n & WidthValue || n & HeightValue)
		fprintf(stderr, "Specified width and height ignored.\n");
	    
	    hints.flags |= PWinGravity;
	    switch ( n& (XNegative | YNegative))
		{
		case 0:
		    hints.win_gravity = NorthWestGravity;
		    break;
		case XNegative:
		    hints.win_gravity = NorthEastGravity;
		    break;
		case YNegative:
		    hints.win_gravity = SouthWestGravity;
		    break;
		default:
		    hints.win_gravity = SouthEastGravity;
		    break;
		}

	    if ((n & XValue) && (n & XNegative))
		x += DisplayWidth(X_display, X_screen) - X_width;
	    if ((n & YValue) && (n & YNegative))
		y += DisplayHeight(X_display, X_screen) - X_height;
	}

    hints.x = x;
    hints.y = y;
    
    /* create the main window */	
    X_mainWindow = XCreateWindow(X_display,
                                 RootWindow(X_display, X_screen),
                                 x, y,
                                 X_width, X_height,
                                 0, /* borderwidth */
                                 X_visualinfo.depth,
                                 InputOutput,
                                 X_visual,
                                 attribmask,
                                 &attribs);
    
    /* Set the Name of the main XWindow */
    XStoreName(X_display, X_mainWindow, "XHeretic ported by A. Werthmann");

    /* Set the Name of the Icon */
    XSetIconName(X_display, X_mainWindow, "XHeretic");

    /* Set window manager hints */
    XSetWMNormalHints(X_display, X_mainWindow, &hints);
    
    
    XDefineCursor(X_display, X_mainWindow,
		  createnullcursor( X_display, X_mainWindow ) );
    
    /* create the GC */
    valuemask = GCGraphicsExposures;
    xgcvalues.graphics_exposures = False;
    X_gc = XCreateGC(X_display,X_mainWindow,valuemask,&xgcvalues);
    
    /* map the window */
    XMapWindow(X_display, X_mainWindow);
    
    /* wait until it is OK to draw */
    oktodraw = 0;
    while (!oktodraw)
      {
	XNextEvent(X_display, &X_event);
	if (X_event.type == Expose
	    && !X_event.xexpose.count)
	  {
	    oktodraw = 1;
	  }
      }
    
    /* grabs the pointer so it is restricted to this window */
    if (grabMouse)
      XGrabPointer(X_display, X_mainWindow, True,
		   ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
		   GrabModeAsync, GrabModeAsync,
		   X_mainWindow, None, CurrentTime);
    
    if (doShm)
      {
	
	X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;
	
	/* create the image */
	image = XShmCreateImage(X_display,
                                X_visual,
                                X_visualinfo.depth,
                                ZPixmap,
                                0,
                                &X_shminfo,
                                X_width,
                                X_height);
	
	grabsharedmemory(image->bytes_per_line * image->height);
	
	
	/*
	 * UNUSED
	 * create the shared memory segment
	 * X_shminfo.shmid = shmget (IPC_PRIVATE,
	 * image->bytes_per_line * image->height, IPC_CREAT | 0777);
	 * if (X_shminfo.shmid < 0)
	 * {
	 * perror("");
	 * I_Error("shmget() failed in InitGraphics()");
	 * }
	 * fprintf(stderr, "shared memory id=%d\n", X_shminfo.shmid);
	 * attach to the shared memory segment
	 * image->data = X_shminfo.shmaddr = shmat(X_shminfo.shmid, 0, 0);
	 */
	
	if (!image->data)
	{
	    perror("");
	    I_Error("shmat() failed in InitGraphics()");
	}
	
	/* get the X server to attach to it */
	if (!XShmAttach(X_display, &X_shminfo))
	  I_Error("XShmAttach() failed in InitGraphics()");
        XShm_is_attached=TRUE;
      }
    else
      {
	  dummy_data = (char*) malloc(X_width * X_height * 4);
	  assert(dummy_data);
	  
	  image = XCreateImage(X_display,
			       X_visual,
			       X_visualinfo.depth,
			       ZPixmap,
			       0,
			       dummy_data,
			       X_width, X_height,
			       (X_visualinfo.depth == 8) ? 8 : 32,
			       0 /*X_width * (X_visualinfo.depth >> 3)*/);
	  
      }
    
    if (multiply == 1 && !true_color)
	screen = (unsigned char *) (image->data);
    else
	{
	    screen = (unsigned char *) malloc (screenwidth * screenheight);
	    assert(screen);
	}    
}


unsigned int exptable[256];

void InitExpand (void)
{
  int i;
  
  for (i=0 ; i<256 ; i++)
    exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

double exptable2[256*256];

void InitExpand2 (void)
{
  int     i;
  int     j;
  double* exp;
  union
  {
    double 		d;
    unsigned int    u[2];
  } pixel;
  
  printf ("building exptable2...\n");
  exp = exptable2;
  for (i=0 ; i<256 ; i++)
    {
      pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
      for (j=0 ; j<256 ; j++)
	{
	  pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	  *exp++ = pixel.d;
	}
    }
  printf ("done.\n");
}

int	inited;

void
Expand4
( unsigned*	lineptr,
  double*	xline )
{
  double	dpixel;
  unsigned	x;
  unsigned 	y;
  unsigned	fourpixels;
  unsigned	step;
  double*	exp;
  
  exp = exptable2;
  if (!inited)
    {
      inited = 1;
      InitExpand2 ();
    }
  
  
  step = 3*screenwidth/2;
  
  y = screenheight-1;
  do
    {
      x = screenwidth;
      
      do
	{
	  fourpixels = lineptr[0];
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[0] = dpixel;
	  xline[160] = dpixel;
	  xline[320] = dpixel;
	  xline[480] = dpixel;
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[1] = dpixel;
	  xline[161] = dpixel;
	  xline[321] = dpixel;
	  xline[481] = dpixel;
	  
	  fourpixels = lineptr[1];
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[2] = dpixel;
	  xline[162] = dpixel;
	  xline[322] = dpixel;
	  xline[482] = dpixel;
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[3] = dpixel;
	  xline[163] = dpixel;
	  xline[323] = dpixel;
	  xline[483] = dpixel;
	  
	  fourpixels = lineptr[2];
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[4] = dpixel;
	  xline[164] = dpixel;
	  xline[324] = dpixel;
	  xline[484] = dpixel;
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[5] = dpixel;
	  xline[165] = dpixel;
	  xline[325] = dpixel;
	  xline[485] = dpixel;
	  
	  fourpixels = lineptr[3];
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[6] = dpixel;
	  xline[166] = dpixel;
	  xline[326] = dpixel;
	  xline[486] = dpixel;
	  
	  dpixel = *(double *)( (long)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[7] = dpixel;
	  xline[167] = dpixel;
	  xline[327] = dpixel;
	  xline[487] = dpixel;
	  
	  lineptr+=4;
	  xline+=8;
	} while (x-=16);
      xline += step;
    } while (y--);
}

void I_CheckRes()
{
  /* nothing to do... */
}


