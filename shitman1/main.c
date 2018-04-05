
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY (0)
#endif

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

typedef uint8_t                 Game_KeyState;

#define Game_KS_DOWN            (1U << 0U)
#define Game_KS_LCTRL           (1U << 1U)
#define Game_KS_RCTRL           (1U << 2U)
#define Game_KS_LALT            (1U << 3U)
#define Game_KS_RALT            (1U << 4U)
#define Game_KS_LSHIFT          (1U << 5U)
#define Game_KS_RSHIFT          (1U << 6U)

#define Game_KS_CTRL            (Game_KS_LCTRL  | Game_KS_RCTRL)
#define Game_KS_ALT             (Game_KS_LALT   | Game_KS_RALT)
#define Game_KS_SHIFT           (Game_KS_LSHIFT | Game_KS_RSHIFT)

#if defined(USING_SDL2)
# define Game_KeyEventQueueMax  256
typedef uint16_t                Game_KeyCode;/*SDL_SCANCODE_*/

# define Game_KC_ESCAPE         SDL_SCANCODE_ESCAPE
# define Game_KC_A              SDL_SCANCODE_A
# define Game_KC_B              SDL_SCANCODE_B
# define Game_KC_C              SDL_SCANCODE_C
# define Game_KC_D              SDL_SCANCODE_D
# define Game_KC_E              SDL_SCANCODE_E
# define Game_KC_F              SDL_SCANCODE_F
# define Game_KC_G              SDL_SCANCODE_G
# define Game_KC_H              SDL_SCANCODE_H
# define Game_KC_I              SDL_SCANCODE_I
# define Game_KC_J              SDL_SCANCODE_J
# define Game_KC_K              SDL_SCANCODE_K
# define Game_KC_L              SDL_SCANCODE_L
# define Game_KC_M              SDL_SCANCODE_M
# define Game_KC_N              SDL_SCANCODE_N
# define Game_KC_O              SDL_SCANCODE_O
# define Game_KC_P              SDL_SCANCODE_P
# define Game_KC_Q              SDL_SCANCODE_Q
# define Game_KC_R              SDL_SCANCODE_R
# define Game_KC_S              SDL_SCANCODE_S
# define Game_KC_T              SDL_SCANCODE_T
# define Game_KC_U              SDL_SCANCODE_U
# define Game_KC_V              SDL_SCANCODE_V
# define Game_KC_W              SDL_SCANCODE_W
# define Game_KC_X              SDL_SCANCODE_X
# define Game_KC_Y              SDL_SCANCODE_Y
# define Game_KC_Z              SDL_SCANCODE_Z
# define Game_KC_1              SDL_SCANCODE_1
# define Game_KC_2              SDL_SCANCODE_2
# define Game_KC_3              SDL_SCANCODE_3
# define Game_KC_4              SDL_SCANCODE_4
# define Game_KC_5              SDL_SCANCODE_5
# define Game_KC_6              SDL_SCANCODE_6
# define Game_KC_7              SDL_SCANCODE_7
# define Game_KC_8              SDL_SCANCODE_8
# define Game_KC_9              SDL_SCANCODE_9
# define Game_KC_0              SDL_SCANCODE_0
# define Game_KC_RETURN         SDL_SCANCODE_RETURN
# define Game_KC_ESCAPE         SDL_SCANCODE_ESCAPE
# define Game_KC_BACKSPACE      SDL_SCANCODE_BACKSPACE
# define Game_KC_TAB            SDL_SCANCODE_TAB
# define Game_KC_SPACE          SDL_SCANCODE_SPACE
# define Game_KC_COMMA          SDL_SCANCODE_COMMA
# define Game_KC_PERIOD         SDL_SCANCODE_PERIOD
# define Game_KC_SLASH          SDL_SCANCODE_SLASH
# define Game_KC_CAPSLOCK       SDL_SCANCODE_CAPSLOCK
# define Game_KC_F1             SDL_SCANCODE_F1
# define Game_KC_F2             SDL_SCANCODE_F2
# define Game_KC_F3             SDL_SCANCODE_F3
# define Game_KC_F4             SDL_SCANCODE_F4
# define Game_KC_F5             SDL_SCANCODE_F5
# define Game_KC_F6             SDL_SCANCODE_F6
# define Game_KC_F7             SDL_SCANCODE_F7
# define Game_KC_F8             SDL_SCANCODE_F8
# define Game_KC_F9             SDL_SCANCODE_F9
# define Game_KC_F10            SDL_SCANCODE_F10
# define Game_KC_F11            SDL_SCANCODE_F11
# define Game_KC_F12            SDL_SCANCODE_F12
# define Game_KC_PRINTSCREEN    SDL_SCANCODE_PRINTSCREEN
# define Game_KC_SCROLLLOCK     SDL_SCANCODE_SCROLLLOCK
# define Game_KC_PAUSE          SDL_SCANCODE_PAUSE
# define Game_KC_INSERT         SDL_SCANCODE_INSERT
# define Game_KC_HOME           SDL_SCANCODE_HOME
# define Game_KC_PAGEUP         SDL_SCANCODE_PAGEUP
# define Game_KC_DELETE         SDL_SCANCODE_DELETE
# define Game_KC_END            SDL_SCANCODE_END
# define Game_KC_PAGEDOWN       SDL_SCANCODE_PAGEDOWN
# define Game_KC_RIGHT          SDL_SCANCODE_RIGHT
# define Game_KC_LEFT           SDL_SCANCODE_LEFT
# define Game_KC_DOWN           SDL_SCANCODE_DOWN
# define Game_KC_UP             SDL_SCANCODE_UP
#else
#error TODO
#endif

typedef struct Game_KeyEvent {
    Game_KeyCode                code;
    Game_KeyState               state;
} Game_KeyEvent;

typedef union palcnvmap {
    uint16_t                    map16[256];
    uint32_t                    map32[256];
} palcnvmap;

enum Game_ResType {
    Game_RT_None=0,
    Game_RT_Other,

    Game_RT_MAX
};

enum Game_ResEnum {
    Game_RE_NULL,

    Game_RE_MAX
};

typedef struct Game_ResEntry {
    uint32_t                    res_size;
#if TARGET_MSDOS == 16
    unsigned char far*          ptr;
#else
    unsigned char*              ptr;
#endif
    uint8_t                     res_type;
    uint8_t                     lockcount;
} Game_ResEntry;

Game_ResEntry                   Game_Res[Game_RE_MAX];

unsigned int                    Game_KeyShiftState;
Game_KeyEvent                   Game_KeyQueue[Game_KeyEventQueueMax];
unsigned int                    Game_KeyQueue_In,Game_KeyQueue_Out;
unsigned int                    Game_ScreenWidth,Game_ScreenHeight;

void Game_KeyShiftStateSet(unsigned int f,unsigned char s) {
    if (s)
        Game_KeyShiftState |=  f;
    else
        Game_KeyShiftState &= ~f;
}

Game_KeyEvent *Game_KeyEvent_Peek(void) {
    if (Game_KeyQueue_Out != Game_KeyQueue_In)
        return Game_KeyQueue + Game_KeyQueue_Out;

    return NULL;
}

Game_KeyEvent *Game_KeyEvent_Get(void) {
    Game_KeyEvent *ev = Game_KeyEvent_Peek();

    if (ev != NULL) {
        if ((++Game_KeyQueue_Out) >= Game_KeyEventQueueMax)
            Game_KeyQueue_Out = 0;
    }

    return ev;
}

Game_KeyEvent *Game_KeyEvent_Add(void) {
    unsigned int nidx = (Game_KeyQueue_In + 1) % Game_KeyEventQueueMax;

    if (nidx != Game_KeyQueue_Out) {
        Game_KeyEvent *ev = Game_KeyQueue + Game_KeyQueue_In;

        memset(ev,0,sizeof(ev));
        Game_KeyQueue_In = nidx;

        return ev;
    }

    return NULL;
}

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

    /* NTS: If you need to render from the middle of your bitmap just adjust bmp */
#if TARGET_MSDOS == 16
    unsigned char far*  bmp;
#else
    unsigned char*      bmp;
#endif
} GAMEBLT;

unsigned int Game_GetTime(void) {
    return SDL_GetTicks();
}

void Game_Delay(const unsigned int ms) {
#if defined(USING_SDL2)
    SDL_Delay(ms);
#endif
}

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

void Game_FatalError(const char *fmt,...) {
    va_list va;

    va_start(va,fmt);
    fprintf(stderr,"Game_FatalError: ");
    vfprintf(stderr,fmt,va);
    fprintf(stderr,"\n");
    va_end(va);

    exit(0);
}

int Game_ResAllocMem(unsigned int i,uint32_t sz,unsigned int type) {
    if (i < Game_RE_MAX && sz != 0UL) {
        if (Game_Res[i].ptr == NULL) {
            Game_Res[i].res_type = type;
            Game_Res[i].res_size = sz;

#if TARGET_MSDOS == 16
            if (sizeof(size_t) == 2 && sz > 0xFFF0UL) return -1; /* perhaps the memory model forces the _fmalloc param to 16-bit wide... */
            Game_Res[i].ptr = _fmalloc((size_t)sz);
#else
            Game_Res[i].ptr = malloc(sz);
#endif
            if (Game_Res[i].ptr != NULL) return 0;
        }
        else if (Game_Res[i].res_size == sz && Game_Res[i].res_type == type) {
            return 0;
        }
    }

    return -1;
}

int Game_ResLoadFile(unsigned int i,uint32_t ofs,uint32_t sz,unsigned int type,const char *path) {
    if (i < Game_RE_MAX) {
        int fd=-1,res=-1;

        if (sz == 0UL) {
            /* sz == 0 means whatever size the file takes */
            struct stat st;

            if (stat(path,&st) != 0)
                return -1;

            /* sanity check */
            if (sizeof(size_t) == 2 && st.st_size > 0xFFF0UL)
                return -1;
            if (st.st_size > 0x1FFF0UL) /* 128KB */
                return -1;

            sz = (uint32_t)st.st_size;
        }

        if (Game_ResAllocMem(i,sz,type) < 0)
            return -1;

        if (Game_Res[i].ptr == NULL)
            Game_FatalError("LoadFile: Resource didn't allocate");

        fd = open(path,O_BINARY|O_RDONLY);
        if (fd >= 0) {
            if (lseek(fd,(off_t)ofs,SEEK_SET) == (off_t)ofs) {
#if TARGET_MSDOS == 16
# error TODO
#else
                if (read(fd,Game_Res[i].ptr,sz) == sz)
                    res = 0;
#endif
            }

            close(fd);
        }

        return res;
    }

    return -1;
}

void Game_ResLock(unsigned int i) {
    if (i < Game_RE_MAX) {
        if (Game_Res[i].ptr != NULL) {
            if (++Game_Res[i].lockcount == 255)
                Game_FatalError("Game_ResLock: Too many locks on resource");
        }
    }
}

void Game_ResUnlock(unsigned int i) {
    if (i < Game_RE_MAX) {
        if (Game_Res[i].ptr != NULL) {
            if (Game_Res[i].lockcount != 0)
                Game_Res[i].lockcount--;
            else
                Game_FatalError("Game_ResUnlock: Attempt to unlock already unlocked resource");
        }
    }
}

void Game_ResFreeMem(unsigned int i) {
    if (i < Game_RE_MAX) {
        if (Game_Res[i].ptr != NULL) {
            if (Game_Res[i].lockcount != 0) Game_FatalError("Attempt to free locked resource mem");
#if TARGET_MSDOS == 16
            _ffree(Game_Res[i].ptr);
#else
            free(Game_Res[i].ptr);
#endif
            Game_Res[i].ptr = NULL;
        }
    }
}

void Game_ResFree(unsigned int i) {
    if (i < Game_RE_MAX) {
        Game_ResFreeMem(i);
        Game_Res[i].res_type = Game_RT_None;
        Game_Res[i].res_size = 0;
    }
}

void Game_ResourceInit(void) {
    memset(&Game_Res,0,sizeof(Game_Res));
}

void Game_ResourceShutdown(void) {
    unsigned int i;

    for (i=0;i < Game_RE_MAX;i++) {
        Game_Res[i].lockcount = 0;
        Game_ResFree(i);
    }
}

int Game_KeyboardInit(void) {
#if defined(USING_SDL2)
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0) {
        fprintf(stderr,"SDL_Init failed, %s\n",SDL_GetError());
        return -1;
    }
#endif

    Game_KeyQueue_In=0;
    Game_KeyQueue_Out=0;
    Game_KeyShiftState=0;
    return 0;
}

void Game_FlushKeyboardQueue(void) {
    Game_KeyQueue_In=0;
    Game_KeyQueue_Out=0;
    Game_KeyShiftState=0;
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

void Game_KeyboardShutdown(void) {
#if defined(USING_SDL2)
    /* SDL ties video and events */
#endif
}

void Game_Idle(void) {
#if defined(USING_SDL2)
    unsigned int i;
    SDL_Event ev;

    for (i=0;i < 16;i++) {
        if (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
                Game_KeyEvent *gev = Game_KeyEvent_Add();

                if (ev.key.keysym.scancode == SDL_SCANCODE_LCTRL)
                    Game_KeyShiftStateSet(Game_KS_LCTRL,ev.key.state == SDL_PRESSED);
                else if (ev.key.keysym.scancode == SDL_SCANCODE_RCTRL)
                    Game_KeyShiftStateSet(Game_KS_RCTRL,ev.key.state == SDL_PRESSED);
                else if (ev.key.keysym.scancode == SDL_SCANCODE_LALT)
                    Game_KeyShiftStateSet(Game_KS_LALT,ev.key.state == SDL_PRESSED);
                else if (ev.key.keysym.scancode == SDL_SCANCODE_RALT)
                    Game_KeyShiftStateSet(Game_KS_RALT,ev.key.state == SDL_PRESSED);
                else if (ev.key.keysym.scancode == SDL_SCANCODE_LSHIFT)
                    Game_KeyShiftStateSet(Game_KS_LSHIFT,ev.key.state == SDL_PRESSED);
                else if (ev.key.keysym.scancode == SDL_SCANCODE_RSHIFT)
                    Game_KeyShiftStateSet(Game_KS_RSHIFT,ev.key.state == SDL_PRESSED);

                if (gev != NULL) {
                    gev->code = ev.key.keysym.scancode;
                    gev->state = ((ev.key.state == SDL_PRESSED) ? Game_KS_DOWN : 0x00) | Game_KeyShiftState;
                }
                else {
                    fprintf(stderr,"SDL: Key event overrun\n");
                }
            }
        }
        else {
            break;
        }
    }
#endif
}

int main(int argc,char **argv) {
    Game_ResourceInit();
    if (Game_KeyboardInit() < 0) {
        fprintf(stderr,"Keyboard init failed\n");
        goto exit;
    }
    if (Game_VideoInit(320,240) < 0) {
        fprintf(stderr,"Video init failed\n");
        goto exit;
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

exit:
    Game_VideoShutdown();
    Game_KeyboardShutdown();
    Game_ResourceShutdown();
    return 0;
}

