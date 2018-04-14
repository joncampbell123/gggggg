
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#include "game.h"
#include "game_ks.h"
#include "game_kc.h"
#include "game_kb.h"
#include "game_tm.h"
#include "game_kev.h"
#include "game_pal.h"

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

void Game_BeginPaletteUpdate(unsigned char idx) {
#if defined(USING_SDL2)
    sdl_palidx = idx;
#endif
}

/* NTS: No guarantee that the change is immediately visible, especially with SDL */
void Game_SetPaletteEntry(unsigned char r,unsigned char g,unsigned char b) {
#if defined(USING_SDL2)
    sdl_screen->format->palette->colors[sdl_palidx].r = r;
    sdl_screen->format->palette->colors[sdl_palidx].g = g;
    sdl_screen->format->palette->colors[sdl_palidx].b = b;
    sdl_screen->format->palette->colors[sdl_palidx].a = 255;

    if (sdl_screen_host->format->BytesPerPixel == 4/*32bpp*/) {
        sdl_palmap.map32[sdl_palidx] =
            ((uint32_t)(r >> sdl_rshiftp) << (uint32_t)sdl_rshift) |
            ((uint32_t)(g >> sdl_gshiftp) << (uint32_t)sdl_gshift) |
            ((uint32_t)(b >> sdl_bshiftp) << (uint32_t)sdl_bshift) |
            (uint32_t)sdl_screen_host->format->Amask;
    }
    else if (sdl_screen_host->format->BytesPerPixel == 2/*16bpp*/) {
        sdl_palmap.map16[sdl_palidx] =
            ((uint16_t)(r >> sdl_rshiftp) << (uint16_t)sdl_rshift) |
            ((uint16_t)(g >> sdl_gshiftp) << (uint16_t)sdl_gshift) |
            ((uint16_t)(b >> sdl_bshiftp) << (uint16_t)sdl_bshift) |
            (uint16_t)sdl_screen_host->format->Amask;
    }
    else {
        /* I doubt SDL2 fully supports 8bpp 256-color paletted */
        fprintf(stderr,"Game set palette entry unsupported format\n");
    }

    sdl_palidx++;
#endif
}

void Game_FinishPaletteUpdates(void) {
#if defined(USING_SDL2)
    /* This code converts from an 8bpp screen so palette animation requires redrawing the whole screen */
    Game_UpdateScreen_All();
#else
    /* this will be a no-op under MS-DOS since our palette writing code will change hardware directly. */
    /* Windows GDI builds will SetDIBitsToDevice here or call RealizePalette if 256-color mode. */
#endif
}

void Game_SetPalette(unsigned char first,unsigned int number,const unsigned char *palette) {
    if ((first+number) > 256)
        return;

    Game_BeginPaletteUpdate(first);

    while (number-- > 0) {
        Game_SetPaletteEntry(palette[0],palette[1],palette[2]);
        palette += 3;
        first++;
    }

    Game_FinishPaletteUpdates();
}

