
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

#include "gif_lib.h"

#if defined(USING_SDL2)
palcnvmap            sdl_palmap;
SDL_Surface*         sdl_screen = NULL;
SDL_Surface*         sdl_screen_host = NULL;
SDL_Window*          sdl_screen_window = NULL;
unsigned char        sdl_rshift,sdl_rshiftp;
unsigned char        sdl_gshift,sdl_gshiftp;
unsigned char        sdl_bshift,sdl_bshiftp;
unsigned char        sdl_palidx=0;
#endif

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

typedef struct GAMEBLT {
    unsigned int        src_h;
    unsigned int        stride;

    /* NTS: If you need to render from the middle of your bitmap just adjust bmp */
#if TARGET_MSDOS == 16
    unsigned char far*  bmp;
#else
    unsigned char*      bmp;
#endif
} GAMEBLT;

void Game_ClearScreen(void) {
#if defined(USING_SDL2)
    unsigned char *row;

    if (SDL_MUSTLOCK(sdl_screen))
        SDL_LockSurface(sdl_screen);

    memset(sdl_screen->pixels,0,sdl_screen->pitch*sdl_screen->h);

    if (SDL_MUSTLOCK(sdl_screen))
        SDL_UnlockSurface(sdl_screen);

    Game_UpdateScreen_All();
#endif
}

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

int main(int argc,char **argv) {
    if (Game_KeyboardInit() < 0) {
        fprintf(stderr,"Keyboard init failed\n");
        Game_Shutdown();
        return 1;
    }
    if (Game_VideoInit(320,240) < 0) {
        fprintf(stderr,"Video init failed\n");
        Game_Shutdown();
        return 1;
    }

    {
        GifFileType *gif;

        gif = LoadGIF("title3.gif");
        if (gif) {
            SavedImage *s = &gif->SavedImages;
            GAMEBLT blt;

            {
                unsigned char pal[768];
                unsigned int i;

                for (i=0;i < 256;i++) {
                    pal[i*3 + 0] = gif->SColorMap.Colors[i].Red;
                    pal[i*3 + 1] = gif->SColorMap.Colors[i].Green;
                    pal[i*3 + 2] = gif->SColorMap.Colors[i].Blue;
                }

                Game_SetPalette(0,256,pal);
            }

            blt.src_h = s->ImageDesc.Height;
            blt.stride = s->ImageDesc.Width;
            blt.bmp = s->RasterBits;

            Game_BitBlt(
                (Game_ScreenWidth - s->ImageDesc.Width) / 2,
                (Game_ScreenHeight - s->ImageDesc.Height) / 2,
                s->ImageDesc.Width,s->ImageDesc.Height,&blt);

            FreeGIF(gif);

            do {
                Game_KeyEvent *ev = Game_KeyEvent_Get();

                if (ev != NULL && !(ev->state & Game_KS_DOWN)) {
                    if (ev->code == Game_KC_RETURN)
                        break;
                }

                Game_Idle();
            } while (1);
        }
    }

    {
        unsigned char pal[768];
        unsigned int i;

        for (i=0;i < 256;i++) {
            pal[i*3 + 0] =
            pal[i*3 + 1] =
            pal[i*3 + 2] = i;
        }

        Game_SetPalette(0,256,pal);
    }

    {
        unsigned char *bmp;
        GAMEBLT blt;
        int x,y,c;

        bmp = malloc(64*64);
        blt.src_h = 64;
        blt.stride = 64;
        blt.bmp = bmp;

        for (y=0;y < 64;y++) {
            for (x=0;x < 64;x++) {
                bmp[((unsigned int)y * 64U) + (unsigned int)x] = (x ^ y) + (((x^y)&1)*64);
            }
        }

        srand(time(NULL));
        for (c=0;c < 1000;c++) {
            x = ((unsigned int)rand() % Game_ScreenWidth);
            y = ((unsigned int)rand() % Game_ScreenHeight);
            Game_BitBlt(x,y,64,64,&blt);
            Game_Idle();
        }

        do {
            Game_KeyEvent *ev = Game_KeyEvent_Get();

            if (ev != NULL) {
                if (ev->code == Game_KC_ESCAPE)
                    break;
            }

            Game_Idle();
        } while (1);

        free(bmp);
    }

    Game_Shutdown();
    return 0;
}

