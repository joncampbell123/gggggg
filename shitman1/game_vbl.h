
typedef struct GAMEBLT {
    unsigned int        src_h;
    unsigned int        stride;

    /* NTS: If you need to render from the middle of your bitmap just adjust bmp */
#if TARGET_MSDOS == 16
    unsigned char far*  bmp;
#else
    unsigned char*      bmp;
#endif
} GAMEBLT;

void Game_BitBlt(unsigned int x,unsigned int y,unsigned int w,unsigned int h,const GAMEBLT * const blt);

