
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
#include "game_kev.h"

Game_KeyEvent                   Game_KeyQueue[Game_KeyEventQueueMax];
unsigned int                    Game_KeyQueue_In,Game_KeyQueue_Out;

void Game_FlushKeyboardQueue(void) {
    Game_KeyQueue_In=0;
    Game_KeyQueue_Out=0;
    Game_KeyShiftState=0;
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

