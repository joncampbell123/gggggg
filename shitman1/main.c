
#include <stdio.h>

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

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

typedef union palcnvmap {
    uint16_t        map16[256];
    uint32_t        map32[256];
} palcnvmap;

#if defined(USING_SDL2)
palcnvmap           sdl_palmap;
SDL_Surface*        sdl_screen = NULL;
SDL_Surface*        sdl_screen_host = NULL;
SDL_Window*         sdl_screen_window = NULL;
unsigned char       sdl_rshift,sdl_rshiftp;
unsigned char       sdl_gshift,sdl_gshiftp;
unsigned char       sdl_bshift,sdl_bshiftp;
#endif

/* NTS: No guarantee that the change is immediately visible, especially with SDL */
void Game_SetPaletteEntry(unsigned char entry,unsigned char r,unsigned char g,unsigned char b) {
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

void Game_FinishPaletteUpdates(void) {
#if defined(USING_SDL2)
#else
/* this will be a no-op under MS-DOS since our palette writing code will change hardware directly. */
/* If this code ever works with Windows GDI directly, this is where the RealizePalette code will go. */
#endif
}

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
#endif
}

void Game_UpdateScreen_All(void) {
    Game_UpdateScreen(0,0,sdl_screen->w,sdl_screen->h);
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

        {
            unsigned int i;

            for (i=0;i < 256;i++)
                Game_SetPaletteEntry(i,i,i,i);
        }

        {
            unsigned char *row;
            unsigned int x,y;

            if (SDL_MUSTLOCK(sdl_screen))
                SDL_LockSurface(sdl_screen);

            for (y=0;y < sdl_screen->h;y++) {
                row = (unsigned char*)(sdl_screen->pixels) + (y * sdl_screen->pitch);
                for (x=0;x < sdl_screen->w;x++) {
                    row[x] = (x == 0 || y == 0 || x == (sdl_screen->w - 1) || y == (sdl_screen->h - 1)) ? 0 : x ^ y;
                }
            }

            if (SDL_MUSTLOCK(sdl_screen))
                SDL_UnlockSurface(sdl_screen);
        }
    }

    Game_UpdateScreen_All();

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

