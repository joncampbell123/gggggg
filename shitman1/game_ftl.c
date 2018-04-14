
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
#include "game_ftl.h"
#include "game_gen.h"
#include "game_vid.h"

void Game_FatalError(const char *fmt,...) {
    va_list va;

    Game_Shutdown();

    va_start(va,fmt);
    fprintf(stderr,"Game_FatalError: ");
    vfprintf(stderr,fmt,va);
    fprintf(stderr,"\n");
    va_end(va);

    exit(0);
}

