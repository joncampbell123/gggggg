
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

#include "gif_lib.h"

#define Game_SS_Flag_Enabled            (1U << 0U)      /* sprite is enabled (visible) */
#define Game_SS_Flag_Transparent        (1U << 1U)      /* transparent (color 0 is transparent) */
#define Game_SS_Flag_Allocated          (1U << 2U)      /* sprite is allocated */
#define Game_SS_Flag_Dirty              (1U << 3U)      /* sprite needs redraw */

typedef struct game_sprite_t {
    int16_t                 x,y;
    uint16_t                w,h;
    uint16_t                flags;
    uint16_t                stride;
#if TARGET_MSDOS == 16
    unsigned char far*      ptr;
#else
    unsigned char*          ptr;
#endif
} game_sprite_t;

#define Game_SpriteMax      128

typedef uint16_t            game_sprite_index_t;

static const uint16_t       game_sprite_index_none = 0xFFFFU;

game_sprite_t               Game_Sprite[Game_SpriteMax];
uint16_t                    Game_SpriteCount; /* highest index + 1 */
uint16_t                    Game_SpriteAvail; /* next available sprite slot */
uint8_t                     Game_SpriteRedrawScreen;

/* compositing buffer is the same size as the screen! */
#if TARGET_MSDOS == 16
unsigned char far*          Game_SpriteCompBuffer = NULL;
#else
unsigned char*              Game_SpriteCompBuffer = NULL;
#endif
GAMEBLT                     Game_SpriteCompBlt;/* x1 <= x < x2,  y1 <= y < y2 */
game_rectxy_t               Game_SpriteCompUpdate;

game_sprite_t *Game_SpriteSlot(const game_sprite_index_t i) {
    if (i < Game_SpriteCount)
        return &Game_Sprite[i];

    return NULL;
}

void Game_SpriteDirtySlot(game_sprite_index_t r);

void Game_SpriteSlotMove(const game_sprite_index_t i,const int16_t x,const int16_t y) {
    game_sprite_t *slot = Game_SpriteSlot(i);

    if (slot != NULL) {
        if (slot->flags & Game_SS_Flag_Enabled) 
            Game_SpriteDirtySlot(i);

        slot->x = x;
        slot->y = y;

        if (slot->flags & Game_SS_Flag_Enabled) 
            Game_SpriteDirtySlot(i);
    }
}

#if TARGET_MSDOS == 16
void Game_SpriteSlotSetImage(const game_sprite_index_t i,const uint16_t w,const uint16_t h,const uint16_t stride,unsigned char far *const ptr,const uint16_t flags) {
#else
void Game_SpriteSlotSetImage(const game_sprite_index_t i,const uint16_t w,const uint16_t h,const uint16_t stride,unsigned char *const ptr,const uint16_t flags) {
#endif
    game_sprite_t *slot = Game_SpriteSlot(i);

    if (slot != NULL) {
        if (slot->flags & Game_SS_Flag_Enabled) 
            Game_SpriteDirtySlot(i);

        slot->w = w;
        slot->h = h;
        slot->ptr = ptr;
        slot->stride = stride;
        slot->flags &= Game_SS_Flag_Enabled | Game_SS_Flag_Allocated;
        slot->flags |= flags & (~(Game_SS_Flag_Enabled | Game_SS_Flag_Allocated));

        if (slot->flags & Game_SS_Flag_Enabled) 
            Game_SpriteDirtySlot(i);
    }
}

game_sprite_index_t Game_SpriteAllocSlotSearch(const game_sprite_index_t max1/*stop just before this index*/) {
    while (Game_SpriteAvail < max1) {
        game_sprite_index_t r = Game_SpriteAvail++;

        if (!(Game_Sprite[r].flags & Game_SS_Flag_Allocated)) {
            Game_Sprite[r].flags = Game_SS_Flag_Allocated;
            return r;
        }
    }

    return game_sprite_index_none;
}

void memcpy_transparent(unsigned char *d,const unsigned char *s,unsigned int sz) {
    while (sz-- > 0) {
        const unsigned char c = *s++;

        if (c != 0)
            *d++ = c;
        else
             d++;
    }
}

void Game_SpriteDirtySlot(game_sprite_index_t r) {
    if (Game_SpriteCompUpdate.x1 > Game_Sprite[r].x)
        Game_SpriteCompUpdate.x1 = Game_Sprite[r].x;
    if (Game_SpriteCompUpdate.y1 > Game_Sprite[r].y)
        Game_SpriteCompUpdate.y1 = Game_Sprite[r].y;

    {
        int16_t x2 = Game_Sprite[r].x + Game_Sprite[r].w;
        int16_t y2 = Game_Sprite[r].y + Game_Sprite[r].h;

        if (Game_SpriteCompUpdate.x2 < x2)
            Game_SpriteCompUpdate.x2 = x2;
        if (Game_SpriteCompUpdate.y2 < y2)
            Game_SpriteCompUpdate.y2 = y2;
    }

    Game_Sprite[r].flags |= Game_SS_Flag_Dirty;
    Game_SpriteRedrawScreen = 1;
}

void Game_SpriteDrawBltItem(game_sprite_t * const slot) {
    /* assume enabled */
#if TARGET_MSDOS == 16
    unsigned char far *sptr = slot->ptr;
    unsigned char far *dptr = Game_SpriteCompBuffer;
#else
    unsigned char *sptr = slot->ptr;
    unsigned char *dptr = Game_SpriteCompBuffer;
#endif
    int16_t x = slot->x - Game_SpriteCompUpdate.x1;
    int16_t y = slot->y - Game_SpriteCompUpdate.y1;
    int16_t w = slot->w;
    int16_t h = slot->h;

    if (x < 0) {
        sptr += (uint16_t)(-x);
        w += x;
        x = 0;
    }
    if (y < 0) {
        sptr += ((uint16_t)(-y)) * ((uint16_t)slot->stride);
        h += y;
        y = 0;
    }
    if (w <= 0 || h <= 0)
        return;
    if ((uint16_t)(x+w) > Game_SpriteCompBlt.stride)
                     w  = Game_SpriteCompBlt.stride - x;
    if ((uint16_t)(y+h) > Game_SpriteCompBlt.src_h)
                     h  = Game_SpriteCompBlt.src_h - y;

#if 1/*DEBUG*/
    if (w <= 0 || h <= 0)
        Game_FatalError("sprite redraw w == 0 || h == 0");
    if (x < 0 || y < 0)
        Game_FatalError("sprite redraw x < 0 || y < 0");
    if ((x+w) > Game_ScreenWidth || (y+h) > Game_ScreenHeight)
        Game_FatalError("sprite redraw (x+w) > max || (y+h) > max");
#endif

    dptr += (uint16_t)x + ((uint16_t)y * Game_SpriteCompBlt.stride);

    if (slot->flags & Game_SS_Flag_Transparent) {
        while (h-- > 0) {
            memcpy_transparent(dptr,sptr,w);
            sptr += slot->stride;
            dptr += Game_SpriteCompBlt.stride;
        }
    }
    else {
        while (h-- > 0) {
            memcpy(dptr,sptr,w);
            sptr += slot->stride;
            dptr += Game_SpriteCompBlt.stride;
        }
    }
}

void Game_SpriteDrawBlt(void) {
    game_sprite_t *slot;
    unsigned int i;

    for (i=0;i < Game_SpriteCount;i++) {
        slot = &Game_Sprite[i];
        if (slot->flags & Game_SS_Flag_Enabled)
            Game_SpriteDrawBltItem(slot);
    }
}

void Game_SpriteDraw(void) {
    if (Game_SpriteRedrawScreen) {
        Game_SpriteRedrawScreen = 0;

        if (Game_SpriteCompUpdate.x1 < 0)
            Game_SpriteCompUpdate.x1 = 0;
        if (Game_SpriteCompUpdate.y1 < 0)
            Game_SpriteCompUpdate.y1 = 0;
        if (Game_SpriteCompUpdate.x2 > Game_ScreenWidth)
            Game_SpriteCompUpdate.x2 = Game_ScreenWidth;
        if (Game_SpriteCompUpdate.y2 > Game_ScreenHeight)
            Game_SpriteCompUpdate.y2 = Game_ScreenHeight;

        if (Game_SpriteCompUpdate.x1 < Game_SpriteCompUpdate.x2 &&
            Game_SpriteCompUpdate.y1 < Game_SpriteCompUpdate.y2) {
            Game_SpriteCompBlt.src_h = Game_SpriteCompUpdate.y2 - Game_SpriteCompUpdate.y1;
            Game_SpriteCompBlt.stride = Game_SpriteCompUpdate.x2 - Game_SpriteCompUpdate.x1;
            Game_SpriteCompBlt.bmp = Game_SpriteCompBuffer;

            /* sprite rendering */
            Game_SpriteDrawBlt();

            /* update the screen */
            Game_BitBlt(
                Game_SpriteCompUpdate.x1,Game_SpriteCompUpdate.y1,
                Game_SpriteCompBlt.stride,Game_SpriteCompBlt.src_h,
                &Game_SpriteCompBlt);
        }

        Game_SpriteCompUpdate.x1 = Game_SpriteCompUpdate.y1 =  0x7FFF;
        Game_SpriteCompUpdate.x2 = Game_SpriteCompUpdate.y2 = -0x7FFF;
    }
}

void Game_SpriteSlotShow(game_sprite_index_t r) {
    /* add dirty rect if sprite was visible */
    if (!(Game_Sprite[r].flags & Game_SS_Flag_Enabled)) {
        Game_Sprite[r].flags |= Game_SS_Flag_Enabled;
        Game_SpriteDirtySlot(r);
    }
}

void Game_SpriteSlotHide(game_sprite_index_t r) {
    /* add dirty rect if sprite was visible */
    if (Game_Sprite[r].flags & Game_SS_Flag_Enabled) {
        Game_Sprite[r].flags &= ~Game_SS_Flag_Enabled;
        Game_SpriteDirtySlot(r);
    }
}

void Game_SpriteFreeSlot(game_sprite_index_t r) {
    if (r < Game_SpriteCount) {
        /* hide the sprite */
        Game_SpriteSlotHide(r);

        /* clear flags */
        Game_Sprite[r].flags = 0;

        /* if this is just before the SpriteCount, then decrement SpriteCount */
        if ((r+1) == Game_SpriteCount)
            Game_SpriteCount--;

        /* free slot, available */
        Game_SpriteAvail = r;
    }
}

game_sprite_index_t Game_SpriteAllocSlot(void) {
    game_sprite_index_t r;

    /* search from avail to count */
    r = Game_SpriteAllocSlotSearch(Game_SpriteCount);
    if (r == game_sprite_index_none) {
        /* try from the beginning */
        Game_SpriteAvail = 0;
        r = Game_SpriteAllocSlotSearch(Game_SpriteCount);

        /* didn't find one, increment count to make another.
         * it's very likely avail == count at this point. */
        if (r == game_sprite_index_none) {
            if (Game_SpriteCount < Game_SpriteMax)
                r = Game_SpriteCount++;
        }
    }

    /* mark it */
    if (r != game_sprite_index_none)
        Game_Sprite[r].flags = Game_SS_Flag_Allocated;

    return r;
}

void Game_SpriteClearAll(void) {
    Game_SpriteCompUpdate.x1 = Game_SpriteCompUpdate.y1 =  0x7FFF;
    Game_SpriteCompUpdate.x2 = Game_SpriteCompUpdate.y2 = -0x7FFF;
    memset(&Game_SpriteCompBlt,0,sizeof(Game_SpriteCompBlt));
    memset(&Game_Sprite,0,sizeof(Game_Sprite));
    Game_SpriteRedrawScreen = 0;
    Game_SpriteCount = 0;
    Game_SpriteAvail = 0;
}

int Game_SpriteInit(void) {
    Game_SpriteClearAll();

    if (Game_SpriteCompBuffer == NULL)
        Game_SpriteCompBuffer = malloc(Game_ScreenWidth * Game_ScreenHeight);

    return 0;
}

void Game_SpriteShutdown(void) {
    Game_SpriteClearAll();

    if (Game_SpriteCompBuffer != NULL) {
        free(Game_SpriteCompBuffer);
        Game_SpriteCompBuffer = NULL;
    }
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
    if (Game_SpriteInit() < 0) {
        fprintf(stderr,"Sprite init failed\n");
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

                if (ev != NULL && (ev->state & Game_KS_DOWN)) {
                    if (ev->code == Game_KC_RETURN)
                        break;
                }

                Game_Idle();
            } while (1);

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

            if (ev != NULL && (ev->state & Game_KS_DOWN)) {
                if (ev->code == Game_KC_RETURN)
                    break;
            }

            Game_Idle();
        } while (1);

        do {
            Game_KeyEvent *ev = Game_KeyEvent_Get();

            if (ev != NULL && !(ev->state & Game_KS_DOWN)) {
                if (ev->code == Game_KC_RETURN)
                    break;
            }

            Game_Idle();
        } while (1);

        free(bmp);
    }

    Game_Shutdown();
    return 0;
}

