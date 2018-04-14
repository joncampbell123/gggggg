
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
#include "game_vup.h"

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

