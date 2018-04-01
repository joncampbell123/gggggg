
#include <stdio.h>

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

typedef union palcnvmap {
    uint16_t        map16[256];
    uint32_t        map32[256];
} palcnvmap;

static inline unsigned int clamp0(int x) {
    return (x > 0) ? x : 0;
}

static inline unsigned int bitmask2shift(uint32_t x) {
    unsigned int c=0;

    if (x != 0) {
        while ((x&1) == 0) {
            c++;
            x>>=1;
        }
    }

    return c;
}

static inline unsigned int bitmask2width(uint32_t x) {
    unsigned int c=0;

    if (x != 0) {
        while ((x&1) == 0) {
            x>>=1;
        }
        while ((x&1) == 1) {
            c++;
            x>>=1;
        }
    }

    return c;
}

#if defined(USING_SDL2)
static palcnvmap            sdl_palmap;
static SDL_Surface*         sdl_screen = NULL;
static SDL_Surface*         sdl_screen_host = NULL;
static SDL_Window*          sdl_screen_window = NULL;
static unsigned char        sdl_rshift,sdl_rshiftp;
static unsigned char        sdl_gshift,sdl_gshiftp;
static unsigned char        sdl_bshift,sdl_bshiftp;
#endif

/* NTS: No guarantee that the change is immediately visible, especially with SDL */
static void Game_SetPaletteEntry(unsigned char entry,unsigned char r,unsigned char g,unsigned char b) {
#if defined(USING_SDL2)
    sdl_screen->format->palette->colors[entry].r = r;
    sdl_screen->format->palette->colors[entry].g = g;
    sdl_screen->format->palette->colors[entry].b = b;
    sdl_screen->format->palette->colors[entry].a = 255;

    if (sdl_screen_host->format->BytesPerPixel == 4/*32bpp*/) {
        sdl_palmap.map32[entry] =
            ((uint32_t)(r >> sdl_rshiftp) << (uint32_t)sdl_rshift) |
            ((uint32_t)(g >> sdl_gshiftp) << (uint32_t)sdl_gshift) |
            ((uint32_t)(b >> sdl_bshiftp) << (uint32_t)sdl_bshift) |
            (uint32_t)sdl_screen_host->format->Amask;
    }
    else if (sdl_screen_host->format->BytesPerPixel == 2/*16bpp*/) {
        sdl_palmap.map16[entry] =
            ((uint16_t)(r >> sdl_rshiftp) << (uint16_t)sdl_rshift) |
            ((uint16_t)(g >> sdl_gshiftp) << (uint16_t)sdl_gshift) |
            ((uint16_t)(b >> sdl_bshiftp) << (uint16_t)sdl_bshift) |
            (uint16_t)sdl_screen_host->format->Amask;
    }
    else {
        /* I doubt SDL2 fully supports 8bpp 256-color paletted */
        fprintf(stderr,"Game set palette entry unsupported format\n");
    }
#endif
}

static void Game_UpdateScreen(unsigned int x,unsigned int y,unsigned int w,unsigned int h) {
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
                srow = ((unsigned char*)sdl_screen->pixels) + ((dy>>1U) * sdl_screen->pitch);
                drow = ((unsigned char*)sdl_screen_host->pixels) + (dy * sdl_screen_host->pitch);

                for (dx=0;dx < w;dx++)
                    ((uint32_t*)drow)[dx*2U + 0U] =
                    ((uint32_t*)drow)[dx*2U + 1U] =
                    sdl_palmap.map32[srow[dx]];
            }
        }
        else if (sdl_screen_host->format->BytesPerPixel == 2) {
            for (dy=0;dy < (h*2);dy++) {
                srow = ((unsigned char*)sdl_screen->pixels) + ((dy>>1U) * sdl_screen->pitch);
                drow = ((unsigned char*)sdl_screen_host->pixels) + (dy * sdl_screen_host->pitch);

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

static void Game_UpdateScreen_All(void) {
    Game_UpdateScreen(0,0,sdl_screen->w,sdl_screen->h);
}

static void Game_FinishPaletteUpdates(void) {
#if defined(USING_SDL2)
    /* This code converts from an 8bpp screen so palette animation requires redrawing the whole screen */
    Game_UpdateScreen_All();
#else
    /* this will be a no-op under MS-DOS since our palette writing code will change hardware directly. */
    /* Windows GDI builds will SetDIBitsToDevice here or call RealizePalette if 256-color mode. */
#endif
}

typedef struct GAMEBLT {
    unsigned int        src_h;
    unsigned int        stride;
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

void Game_BitBlt(unsigned int x,unsigned int y,unsigned int w,unsigned int h,const GAMEBLT * const blt) {
    if (blt->bmp == NULL)
        return;
    if (w > blt->stride || h > blt->src_h || x >= blt->stride || y >= blt->src_h)
        return;
    if ((x+w) > blt->stride || (y+h) > blt->src_h)
        return;
}

void Game_SetPalette(unsigned char first,unsigned int number,const unsigned char *palette) {
    if ((first+number) > 256)
        return;

    while (number-- > 0) {
        Game_SetPaletteEntry(first,palette[0],palette[1],palette[2]);
        palette += 3;
        first++;
    }

    Game_FinishPaletteUpdates();
}

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

    Game_ClearScreen();

    SDL_Delay(2000);
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
}

int main(int argc,char **argv) {
    if (Game_VideoInit(320,240) < 0) {
        fprintf(stderr,"Video init failed\n");
        Game_VideoShutdown();
        return 1;
    }

    Game_VideoShutdown();
    return 0;
}

