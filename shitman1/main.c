
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

game_rectxy_t               sprite_update;

void Game_SpriteAddUpdateRect(int16_t x,int16_t y,int16_t w,int16_t h) {
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }

    if (w > 0 && h > 0) {
        int16_t x2 = x + w;
        int16_t y2 = y + h;

        if (x2 > Game_ScreenWidth)
            x2 = Game_ScreenWidth;
        if (y2 > Game_ScreenHeight)
            y2 = Game_ScreenHeight;

        if (sprite_update.x1 > x)
            sprite_update.x1 = x;
        if (sprite_update.y1 > y)
            sprite_update.y1 = y;
        if (sprite_update.x2 < x2)
            sprite_update.x2 = x2;
        if (sprite_update.y2 < y2)
            sprite_update.y2 = y2;
    }
}

void Game_SpriteClearUpdate(void) {
    sprite_update.x1 =  0x7FFF;
    sprite_update.y1 =  0x7FFF;
    sprite_update.x2 = -0x7FFF;
    sprite_update.y2 = -0x7FFF;
}

int Game_SpriteInit(void) {
    Game_SpriteClearUpdate();
    return 0;
}

void Game_SpriteShutdown(void) {
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

