
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
#include "game_tm.h"
#include "game_kev.h"

unsigned int Game_GetTime(void) {
#if defined(USING_SDL2)
    return SDL_GetTicks();
#else
    return 0;
#endif
}

void Game_Delay(const unsigned int ms) {
#if defined(USING_SDL2)
    SDL_Delay(ms);
#endif
}

