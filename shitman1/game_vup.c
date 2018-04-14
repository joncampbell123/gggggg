
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
#include "game_vup.h"

void Game_UpdateScreen(unsigned int x,unsigned int y,unsigned int w,unsigned int h) {
#if defined(USING_SDL2)
    SDL_Rect dst;

    /* Well, SDL2 doesn't seem to offer a "blit and convert 8bpp to host screen"
     * so we'll just do it ourselves */

    /* Note that this game is designed around 320x240 or 320x200 256-color VGA mode.
     * To display properly for modern users we have to scale up. */

    dst.x = x * 2;
    dst.y = y * 2;
    dst.w = w * 2;
    dst.h = h * 2;

    if ((x+w) > sdl_screen->w || (y+h) > sdl_screen->h) {
        fprintf(stderr,"updatescreen invalid rect x=%u,y=%u,w=%u,h=%u\n",x,y,w,h);
        return;
    }

    if (((x+w)*2) > sdl_screen_host->w || ((y+h)*2) > sdl_screen_host->h) {
        fprintf(stderr,"updatescreen invalid rect for screen\n");
        return;
    }

    {
        unsigned char *srow;
        unsigned char *drow;
        unsigned int dx,dy;

        if (SDL_MUSTLOCK(sdl_screen))
            SDL_LockSurface(sdl_screen);
        if (SDL_MUSTLOCK(sdl_screen_host))
            SDL_LockSurface(sdl_screen_host);

        if (sdl_screen_host->format->BytesPerPixel == 4) {
            for (dy=0;dy < (h*2);dy++) {
                srow = ((unsigned char*)sdl_screen->pixels) + (((dy>>1U) + y) * sdl_screen->pitch) + x;
                drow = ((unsigned char*)sdl_screen_host->pixels) + ((dy + (y<<1U)) * sdl_screen_host->pitch) + (x<<(1U+2U));

                for (dx=0;dx < w;dx++)
                    ((uint32_t*)drow)[dx*2U + 0U] =
                    ((uint32_t*)drow)[dx*2U + 1U] =
                    sdl_palmap.map32[srow[dx]];
            }
        }
        else if (sdl_screen_host->format->BytesPerPixel == 2) {
            for (dy=0;dy < (h*2);dy++) {
                srow = ((unsigned char*)sdl_screen->pixels) + (((dy>>1U) + y) * sdl_screen->pitch) + x;
                drow = ((unsigned char*)sdl_screen_host->pixels) + ((dy + (y<<1U)) * sdl_screen_host->pitch) + (x<<(1U+1U));

                for (dx=0;dx < w;dx++)
                    ((uint16_t*)drow)[dx*2U + 0U] =
                    ((uint16_t*)drow)[dx*2U + 1U] =
                    sdl_palmap.map16[srow[dx]];
            }
        }
        else {
            fprintf(stderr,"GameUpdate not supported format\n");
        }

        if (SDL_MUSTLOCK(sdl_screen))
            SDL_UnlockSurface(sdl_screen);
        if (SDL_MUSTLOCK(sdl_screen_host))
            SDL_UnlockSurface(sdl_screen_host);
    }

    /* Let SDL know */
    if (SDL_UpdateWindowSurfaceRects(sdl_screen_window,&dst,1) != 0)
        fprintf(stderr,"updatewindow err\n");
#else
    /* This will be a no-op under MS-DOS since blitting will be done directly to the screen */
    /* Windows GDI builds will SetDIBitsToDevice here */
#endif
}

void Game_UpdateScreen_All(void) {
    Game_UpdateScreen(0,0,sdl_screen->w,sdl_screen->h);
}

