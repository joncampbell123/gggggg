
#ifndef O_BINARY
#define O_BINARY (0)
#endif

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

typedef struct game_rectwh_t {
    int16_t             x;
    int16_t             y;
    int16_t             w;
    int16_t             h;
} game_rectwh_t;

/* rect is defined to cover x1 <= x < x2
 *                      and y1 <= y < y2 */
typedef struct game_rectxy_t {
    int16_t             x1;
    int16_t             y1;
    int16_t             x2;
    int16_t             y2;
} game_rectxy_t;

