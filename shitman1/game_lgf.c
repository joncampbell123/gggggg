
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

#include "gif_lib.h"

GifFileType *FreeGIF(GifFileType *gif) {
    int err;

    if (gif) DGifCloseFile(gif,&err);

    return NULL;
}

GifFileType *LoadGIF(const char *path) {
    GifFileType *gif;
    int err;

    gif = DGifOpenFileName(path,&err);
    if (gif == NULL) return NULL;

    /* TODO: How do we read only the first image? */
    if (DGifSlurp(gif) != GIF_OK) {
        DGifCloseFile(gif,&err);
        return NULL;
    }
    if (gif->SavedImages.RasterBits == NULL) {
        DGifCloseFile(gif,&err);
        return NULL;
    }

    return gif;
}

