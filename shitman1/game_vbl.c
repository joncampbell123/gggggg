
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>

#include "game.h"
#include "game_ks.h"
#include "game_kc.h"
#include "game_kb.h"
#include "game_tm.h"
#include "game_vc.h"
#include "game_kev.h"
#include "game_pal.h"
#include "game_utl.h"
#include "game_lgf.h"
#include "game_idl.h"
#include "game_ftl.h"
#include "game_gen.h"
#include "game_vid.h"
#include "game_vbl.h"

#if defined(USING_SDL2)
extern palcnvmap            sdl_palmap;
extern SDL_Surface*         sdl_screen;
extern SDL_Surface*         sdl_screen_host;
extern SDL_Window*          sdl_screen_window;
extern unsigned char        sdl_rshift,sdl_rshiftp;
extern unsigned char        sdl_gshift,sdl_gshiftp;
extern unsigned char        sdl_bshift,sdl_bshiftp;
extern unsigned char        sdl_palidx;
#endif

void Game_UpdateScreen(unsigned int x,unsigned int y,unsigned int w,unsigned int h);

/* this checks the x,y,w,h values against the screen, does NOT clip but instead rejects.
 * the game engine is supposed to compose to the memory buffer (with clipping) and then call this function */
void Game_BitBlt(unsigned int x,unsigned int y,unsigned int w,unsigned int h,const GAMEBLT * const blt) {
    if (blt->bmp == NULL)
        return;

    if ((x+w) > Game_ScreenWidth || (y+h) > Game_ScreenHeight) return;
    if ((x|y|w|h) & (~0x7FFFU)) return; // negative coords, anything >= 32768
    if (w == 0 || h == 0) return;

#if 1/*DEBUG*/
    if (x >= Game_ScreenWidth || y >= Game_ScreenHeight) abort();
    if ((x+w) > Game_ScreenWidth || (y+h) > Game_ScreenHeight) abort();
#endif

#if TARGET_MSDOS == 16
#error TODO
#else
    {
        unsigned char *srow;
        unsigned char *drow;
        unsigned int dy;

        if (SDL_MUSTLOCK(sdl_screen))
            SDL_LockSurface(sdl_screen);

#if 0/*DEBUG to check update rect*/
        memset(sdl_screen->pixels,0,sdl_screen->pitch*sdl_screen->h);
#endif

        srow = blt->bmp;
        drow = ((unsigned char*)sdl_screen->pixels) + (y * sdl_screen->pitch) + x;
        for (dy=0;dy < h;dy++) {
            memcpy(drow,srow,w);
            drow += sdl_screen->pitch;
            srow += blt->stride;
        }

        if (SDL_MUSTLOCK(sdl_screen))
            SDL_UnlockSurface(sdl_screen);
    }
#endif

    Game_UpdateScreen(x,y,w,h);
}

