
#ifndef O_BINARY
#define O_BINARY (0)
#endif

#if defined(LINUX)
# define USING_SDL2
#endif

#if defined(USING_SDL2)
# include <SDL2/SDL.h>
#endif

