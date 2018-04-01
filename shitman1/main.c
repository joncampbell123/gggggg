
#include <stdio.h>

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

#if defined(USING_SDL2)
SDL_Surface*        sdl_screen = NULL;
SDL_Window*         sdl_screen_window = NULL;
SDL_Renderer*       sdl_screen_renderer = NULL;
#endif

int Game_VideoInit(void) {
#if defined(USING_SDL2)
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr,"SDL_Init failed, %s\n",SDL_GetError());
        return -1;
    }
    if (sdl_screen_window == NULL) {
        sdl_screen_window = SDL_CreateWindow(
            "Shitman"/*title*/,
            SDL_WINDOWPOS_UNDEFINED/*x*/,
            SDL_WINDOWPOS_UNDEFINED/*y*/,
            640/*w*/,
            480/*h*/,
            0/*flags*/);
        if (sdl_screen_window == NULL)
            return -1;

        sdl_screen = SDL_GetWindowSurface(sdl_screen_window);
        if (sdl_screen == NULL)
            return -1;
    }
    if (sdl_screen_renderer == NULL) {
        sdl_screen_renderer = SDL_CreateRenderer(
            sdl_screen_window/*window*/,
            -1/*rendering driver*/,
            0/*flags*/);
        if (sdl_screen_renderer == NULL)
            return -1;
    }

    SDL_SetRenderDrawColor(sdl_screen_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // black
    SDL_RenderClear(sdl_screen_renderer);
    SDL_RenderPresent(sdl_screen_renderer);
#endif

    return 0;
}

void Game_VideoShutdown(void) {
#if defined(USING_SDL2)
    sdl_screen = NULL;
    if (sdl_screen_renderer != NULL) {
        SDL_DestroyRenderer(sdl_screen_renderer);
        sdl_screen_renderer = NULL;
    }
    if (sdl_screen_window != NULL) {
        SDL_DestroyWindow(sdl_screen_window);
        sdl_screen_window = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
}

int main(int argc,char **argv) {
    if (Game_VideoInit() < 0) {
        Game_VideoShutdown();
        return 1;
    }

    Game_VideoShutdown();
    return 0;
}

