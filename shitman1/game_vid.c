
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
#include "game_kev.h"
#include "game_pal.h"
#include "game_utl.h"
#include "game_lgf.h"
#include "game_idl.h"
#include "game_ftl.h"
#include "game_gen.h"
#include "game_vid.h"

void Game_ClearScreen(void);

unsigned int            Game_ScreenWidth,Game_ScreenHeight;

#if defined(USING_SDL2)
palcnvmap               sdl_palmap;
SDL_Surface*            sdl_screen = NULL;
SDL_Surface*            sdl_screen_host = NULL;
SDL_Window*             sdl_screen_window = NULL;
unsigned char           sdl_rshift,sdl_rshiftp;
unsigned char           sdl_gshift,sdl_gshiftp;
unsigned char           sdl_bshift,sdl_bshiftp;
unsigned char           sdl_palidx=0;
#endif

int Game_VideoInit(unsigned int screen_w,unsigned int screen_h) {
#if defined(USING_SDL2)
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr,"SDL_Init failed, %s\n",SDL_GetError());
        return -1;
    }
    if (sdl_screen_window == NULL) {
        sdl_screen_window = SDL_CreateWindow(
            "Shitman"/*title*/,
            SDL_WINDOWPOS_UNDEFINED/*x*/,
            SDL_WINDOWPOS_UNDEFINED/*y*/,
            (screen_w*2)/*w*/,
            (screen_h*2)/*h*/,
            0/*flags*/);
        if (sdl_screen_window == NULL)
            return -1;
    }
    if (sdl_screen_host == NULL) {
        sdl_screen_host = SDL_GetWindowSurface(sdl_screen_window);
        if (sdl_screen_host == NULL)
            return -1;

        /* This game (through SDL2) only supports displays using 16bpp or 32bpp RGB(A) formats.
         * 24bpp is very uncommon these days, and nobody runs modern systems in 256-color anymore.
         * Perhaps in future game design I might make an alternate Windows build that uses
         * Windows GDI directly to support Windows 98 or lower (down to Windows 3.1), but not right now. */
        if (sdl_screen_host->format->BytesPerPixel == 4/*32bpp*/) {
            /* OK */
        }
        else if (sdl_screen_host->format->BytesPerPixel == 2/*16bpp*/) {
            /* OK */
        }
        else {
            fprintf(stderr,"SDL2 error: Bytes/pixel %u rejected\n",
                sdl_screen_host->format->BytesPerPixel);
            return -1;
        }

        if (SDL_PIXELTYPE(sdl_screen_host->format->format) == SDL_PIXELTYPE_PACKED16 ||
            SDL_PIXELTYPE(sdl_screen_host->format->format) == SDL_PIXELTYPE_PACKED32) {
        }
        else {
            fprintf(stderr,"SDL2 error: Pixel type format rejected (must be packed)\n");
            return -1;
        }
    }
    if (sdl_screen == NULL) {
        sdl_screen = SDL_CreateRGBSurface(
            0/*flags*/,
            screen_w/*w*/,
            screen_h/*h*/,
            8/*depth*/,
            0,0,0,0/*RGBA mask*/);
        if (sdl_screen == NULL)
            return -1;
        if (sdl_screen->format == NULL)
            return -1;
        if (sdl_screen->format->palette == NULL)
            return -1;
        if (sdl_screen->format->palette->ncolors < 256)
            return -1;
        if (sdl_screen->format->palette->colors == NULL)
            return -1;

        sdl_rshift = bitmask2shift(sdl_screen_host->format->Rmask);
        sdl_gshift = bitmask2shift(sdl_screen_host->format->Gmask);
        sdl_bshift = bitmask2shift(sdl_screen_host->format->Bmask);

        sdl_rshiftp = clamp0(8 - bitmask2width(sdl_screen_host->format->Rmask));
        sdl_gshiftp = clamp0(8 - bitmask2width(sdl_screen_host->format->Gmask));
        sdl_bshiftp = clamp0(8 - bitmask2width(sdl_screen_host->format->Bmask));

        fprintf(stderr,"Screen format:\n");
        fprintf(stderr,"  Red:    shift pre=%u post=%u\n",sdl_rshiftp,sdl_rshift);
        fprintf(stderr,"  Green:  shift pre=%u post=%u\n",sdl_gshiftp,sdl_gshift);
        fprintf(stderr,"  Blue:   shift pre=%u post=%u\n",sdl_bshiftp,sdl_bshift);
    }

    Game_ScreenWidth = screen_w;
    Game_ScreenHeight = screen_h;

    Game_ClearScreen();
#endif

    return 0;
}

void Game_VideoShutdown(void) {
#if defined(USING_SDL2)
    sdl_screen_host = NULL;
    if (sdl_screen != NULL) {
        SDL_FreeSurface(sdl_screen);
        sdl_screen = NULL;
    }
    if (sdl_screen_window != NULL) {
        SDL_DestroyWindow(sdl_screen_window);
        sdl_screen_window = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif

    Game_ScreenWidth = 0;
    Game_ScreenHeight = 0;
}

