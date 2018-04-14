
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

unsigned int                Game_KeyShiftState;

void Game_KeyShiftStateSet(unsigned int f,unsigned char s) {
    if (s)
        Game_KeyShiftState |=  f;
    else
        Game_KeyShiftState &= ~f;
}

