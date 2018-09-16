
/* SDL graphics-module was taken from the Linux-Hexen port */
/* and modified to work with Heretic */

#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include "doomdef.h"

#include "SDL/SDL.h"


/* Public Data */

static int DisplayTicker = 0;
static int ticcount;

SDL_Surface* sdl_screen;
int grabMouse;

/*
 *--------------------------------------------------------------------------
 *
 * PROC I_SetPalette
 *
 * Palette source must use 8 bit RGB elements.
 *
 *--------------------------------------------------------------------------
 */

void I_SetPalette(byte *palette)
{
    SDL_Color* c;
    SDL_Color* cend;
    SDL_Color cmap[ 256 ];
    
    I_WaitVBL(1);
    
    c = cmap;
    cend = c + 256;
    for( ; c != cend; c++ )
	{
	    c->r = gammatable[usegamma][*palette++];
	    c->g = gammatable[usegamma][*palette++];
	    c->b = gammatable[usegamma][*palette++];
	}
    SDL_SetColors( sdl_screen, cmap, 0, 256 );
}

/*
============================================================================

							GRAPHICS MODE

============================================================================
*/

byte *destview;

/*
==============
=
= I_Update
=
==============
*/

int UpdateState;
extern int screenblocks;

void I_FinishUpdate (void)
{
    int i;
    byte *dest;
    int tics;
    static int lasttic;

    /*
     * blit screen to video
     */
    if(DisplayTicker) {
	dest = (byte *)screen;
	
	tics = ticcount-lasttic;
	lasttic = ticcount;
	if(tics > 20) {
	    tics = 20;
	}
	for(i = 0; i < tics; i++) {
	    *dest = 0xff;
	    dest += 2;
	}
	for(i = tics; i < 20; i++) {
	    *dest = 0x00;
	    dest += 2;
	}
    }
    SDL_UpdateRect( sdl_screen, 0, 0, screenwidth, screenheight );
}

void InitGraphLib(void) {
    if( SDL_Init( SDL_INIT_VIDEO) < 0 ) {
	I_Error("Not running in graphics capable console or could not find a free VC\n");
    }
}

/*
 *--------------------------------------------------------------------------
 *
 * PROC I_InitGraphics
 *
 *--------------------------------------------------------------------------
 */

void I_InitGraphics(void) {
    
    ticcount = (SDL_GetTicks()*35)/1000;
    
    /*
     * SDL_DOUBLEBUF does not work in full screen mode.  Does not seem to
     * be necessary anyway.
     */
    
    sdl_screen = SDL_SetVideoMode( screenwidth, screenheight, 8,
				   /*SDL_FULLSCREEN*/0 );
    /* SDL_HWSURFACE | SDL_FULLSCREEN ); */
    /* 0 ); */
    if( sdl_screen == NULL )
	{
	    fprintf( stderr, "Couldn't set video mode %dx%d: %s\n",
		     screenwidth, screenheight, SDL_GetError() );
	    exit( 3 );
	}
    
    if( SDL_MUSTLOCK( sdl_screen ) )
	{
	    printf( "SDL_MUSTLOCK\n" );
	    exit( 4 );
	}
    
    if ( M_CheckParm("-grabmouse") ) {
	grabMouse = 1;
    } else {
	    grabMouse = 0;
    }
    SDL_ShowCursor( 0 );
    SDL_WM_SetCaption( "SDLHeretic v1.03", "Heretic" );
    
    I_SetPalette( W_CacheLumpName("PLAYPAL", PU_CACHE) );

    screen = malloc(screenheight*screenwidth);
    if (!screen) {
	I_Error("Couldn't allocate space for screenmemory !\n");
    }
    sdl_screen->pixels = screen;
}

/*
 *--------------------------------------------------------------------------
 *
 * PROC I_ShutdownGraphics
 *
 *--------------------------------------------------------------------------
 */

void I_ShutdownGraphics(void)
{
	/* Oh honestly... what idiotic programmers would fail to
	 * clean up SDL state when their done? Freaking morons... --Jonathan C */
	SDL_Quit();
}

void I_CheckRes()
{
  /* nothing to do... */
}

/*
 *  Translates the key 
 */

int xlatekey(SDL_keysym *key)
{
    int rc;
    
    switch(key->sym)
	{
	case SDLK_LEFT:	rc = KEY_LEFTARROW;	break;
	case SDLK_RIGHT:	rc = KEY_RIGHTARROW;	break;
	case SDLK_DOWN:	rc = KEY_DOWNARROW;	break;
	case SDLK_UP:	rc = KEY_UPARROW;	break;
	case SDLK_ESCAPE:	rc = KEY_ESCAPE;	break;
	case SDLK_RETURN:	rc = KEY_ENTER;		break;
	case SDLK_F1:	rc = KEY_F1;		break;
	case SDLK_F2:	rc = KEY_F2;		break;
	case SDLK_F3:	rc = KEY_F3;		break;
	case SDLK_F4:	rc = KEY_F4;		break;
	case SDLK_F5:	rc = KEY_F5;		break;
	case SDLK_F6:	rc = KEY_F6;		break;
	case SDLK_F7:	rc = KEY_F7;		break;
	case SDLK_F8:	rc = KEY_F8;		break;
	case SDLK_F9:	rc = KEY_F9;		break;
	case SDLK_F10:	rc = KEY_F10;		break;
	case SDLK_F11:	rc = KEY_F11;		break;
	case SDLK_F12:	rc = KEY_F12;		break;
	    
	case SDLK_INSERT: rc = KEY_INSERT;           break;
	case SDLK_DELETE: rc = KEY_DELETE;           break;
	case SDLK_PAGEUP: rc = KEY_PAGEUP;          break;
	case SDLK_PAGEDOWN: rc = KEY_PAGEDOWN;        break;
	case SDLK_HOME:   rc = KEY_HOME;          break;
	case SDLK_END:    rc = KEY_END;           break;
	    
	case SDLK_BACKSPACE: rc = KEY_BACKSPACE;	break;
	    
	case SDLK_PAUSE:	rc = KEY_PAUSE;		break;
	    
	case SDLK_EQUALS:	rc = KEY_EQUALS;	break;
	    
	case SDLK_KP_MINUS:
	case SDLK_MINUS:	rc = KEY_MINUS;		break;

	case SDLK_KP_PLUS:
	case SDLK_PLUS:         rc = KEY_EQUALS;        break;
	    
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
	    rc = KEY_RSHIFT;
	    break;
	    
	case SDLK_LCTRL:
	case SDLK_RCTRL:
	    rc = KEY_RCTRL;
	    break;
	    
	case SDLK_LALT:
	case SDLK_LMETA:
	case SDLK_RALT:
	case SDLK_RMETA:
	    rc = KEY_RALT;
	    break;
	    
	default:
	    rc = key->sym;
	    break;
	}
    return rc;
}


/* This processes SDL events */
void I_GetEvent(SDL_Event *Event)
{
    Uint8 buttonstate;
    event_t event;

    switch (Event->type)
	{
	case SDL_KEYDOWN:
	    event.type = ev_keydown;
	    event.data1 = xlatekey(&Event->key.keysym);
	    D_PostEvent(&event);
	    break;
	    
	case SDL_KEYUP:
	    event.type = ev_keyup;
	    event.data1 = xlatekey(&Event->key.keysym);
	    D_PostEvent(&event);
	    break;
	    
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	    buttonstate = SDL_GetMouseState(NULL, NULL);
	    event.type = ev_mouse;
	    event.data1 = 0
		| (buttonstate & SDL_BUTTON(1) ? 1 : 0)
		| (buttonstate & SDL_BUTTON(2) ? 2 : 0)
		| (buttonstate & SDL_BUTTON(3) ? 4 : 0);
	    event.data2 = event.data3 = 0;
	    D_PostEvent(&event);
	    break;
	    
#if (SDL_MAJOR_VERSION >= 0) && (SDL_MINOR_VERSION >= 9)
	case SDL_MOUSEMOTION:
	    /* Ignore mouse warp events */
	    if ( (Event->motion.x != sdl_screen->w/2) ||
		 (Event->motion.y != sdl_screen->h/2) )
		{
		    /* Warp the mouse back to the center */
		    if (grabMouse) {
			SDL_WarpMouse(sdl_screen->w/2, sdl_screen->h/2);
		    }
		    event.type = ev_mouse;
		    event.data1 = 0
			| (Event->motion.state & SDL_BUTTON(1) ? 1 : 0)
			| (Event->motion.state & SDL_BUTTON(2) ? 2 : 0)
			| (Event->motion.state & SDL_BUTTON(3) ? 4 : 0);
		    event.data2 = Event->motion.xrel << 2;
		    event.data3 = -Event->motion.yrel << 2;
		    D_PostEvent(&event);
		}
	    break;
#endif
	    
	case SDL_QUIT:
	    I_Quit();
	}
}

/*
 * I_StartTic
 */
void I_StartTic (void)
{
    SDL_Event Event;
    
    while ( SDL_PollEvent(&Event) )
        I_GetEvent(&Event);
}

/*
===============
=
= I_StartFrame
=
===============
*/

void I_StartFrame (void)
{
    /* ? */
}
