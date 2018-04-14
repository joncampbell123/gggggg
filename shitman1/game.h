
#ifndef O_BINARY
#define O_BINARY (0)
#endif

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

typedef struct game_rect_t {
    int16_t             x;
    int16_t             y;
    int16_t             w;
    int16_t             h;
} game_rect_t;

