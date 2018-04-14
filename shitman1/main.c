
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
#include "game_spr.h"

#include "gif_lib.h"

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
        game_sprite_index_t spr,spr2,spr3;
        unsigned char *bmp;
        GAMEBLT blt;
        int x,y,c;
        double a;

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

        /* sprite rendering test */
        Game_SpriteDraw();

        spr = Game_SpriteAllocSlot();
        if (spr == game_sprite_index_none) abort();

        spr2 = Game_SpriteAllocSlot();
        if (spr2 == game_sprite_index_none) abort();

        spr3 = Game_SpriteAllocSlot();
        if (spr3 == game_sprite_index_none) abort();

        Game_SpriteSlotSetImage(spr,64,64,64,bmp,0);
        Game_SpriteSlotMove(spr,0,0);
        Game_SpriteSlotShow(spr);

        Game_SpriteSlotSetImage(spr2,64,64,64,bmp,0);
        Game_SpriteSlotMove(spr2,32,32);
        Game_SpriteSlotShow(spr2);

        Game_SpriteSlotSetImage(spr3,64,64,64,bmp,0);
        Game_SpriteSlotMove(spr2,64,64);
        Game_SpriteSlotShow(spr3);

        Game_SpriteDraw();

        a = 0;
        do {
            Game_KeyEvent *ev = Game_KeyEvent_Get();

            x  = (Game_ScreenWidth / 2) - (64/2);
            x += ((Game_ScreenWidth / 2) + 32) * sin(a);
            y  = (Game_ScreenHeight / 2) - (64/2);
            y += ((Game_ScreenHeight / 2) + 32) * cos(a);
            Game_SpriteSlotMove(spr,x,y);

            x  = (Game_ScreenWidth / 2) - (64/2);
            x += ((Game_ScreenWidth / 2) - 0) * sin(a*0.99);
            y  = (Game_ScreenHeight / 2) - (64/2);
            y += ((Game_ScreenHeight / 2) - 0) * cos(a*0.99);
            Game_SpriteSlotMove(spr2,x,y);

            x  = (Game_ScreenWidth / 2) - (64/2);
            x += ((Game_ScreenWidth / 2) - 32) * sin(a*0.98);
            y  = (Game_ScreenHeight / 2) - (64/2);
            y += ((Game_ScreenHeight / 2) - 32) * cos(a*0.98);
            Game_SpriteSlotMove(spr3,x,y);

            Game_SpriteDraw();

            if (ev != NULL && (ev->state & Game_KS_DOWN)) {
                if (ev->code == Game_KC_RETURN)
                    break;
            }

            Game_Idle();
            Game_Delay(1000 / 60);

            a += M_PI / (60 * 1);
        } while (1);

        do {
            Game_KeyEvent *ev = Game_KeyEvent_Get();

            if (ev != NULL && !(ev->state & Game_KS_DOWN)) {
                if (ev->code == Game_KC_RETURN)
                    break;
            }

            Game_Idle();
        } while (1);

        Game_SpriteFreeSlot(spr3);
        Game_SpriteFreeSlot(spr2);
        Game_SpriteFreeSlot(spr);
        Game_SpriteDraw();

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

