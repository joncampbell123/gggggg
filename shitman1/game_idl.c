
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

