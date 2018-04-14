
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
#include "game_kb.h"
#include "game_kev.h"

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

void Game_KeyboardShutdown(void) {
#if defined(USING_SDL2)
    /* SDL ties video and events */
#endif
}

