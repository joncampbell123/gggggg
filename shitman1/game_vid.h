
#if defined(USING_SDL2)
extern palcnvmap            sdl_palmap;
extern SDL_Surface*         sdl_screen;
extern SDL_Surface*         sdl_screen_host;
extern SDL_Window*          sdl_screen_window;
extern unsigned char        sdl_rshift,sdl_rshiftp;
extern unsigned char        sdl_gshift,sdl_gshiftp;
extern unsigned char        sdl_bshift,sdl_bshiftp;
extern unsigned char        sdl_palidx;
#endif

int Game_VideoInit(unsigned int screen_w,unsigned int screen_h);
void Game_VideoShutdown(void);

extern unsigned int         Game_ScreenWidth,Game_ScreenHeight;

