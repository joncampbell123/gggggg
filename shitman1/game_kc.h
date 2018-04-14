
#if defined(USING_SDL2)
# define Game_KeyEventQueueMax  256
typedef uint16_t                Game_KeyCode;/*SDL_SCANCODE_*/

# define Game_KC_ESCAPE         SDL_SCANCODE_ESCAPE
# define Game_KC_A              SDL_SCANCODE_A
# define Game_KC_B              SDL_SCANCODE_B
# define Game_KC_C              SDL_SCANCODE_C
# define Game_KC_D              SDL_SCANCODE_D
# define Game_KC_E              SDL_SCANCODE_E
# define Game_KC_F              SDL_SCANCODE_F
# define Game_KC_G              SDL_SCANCODE_G
# define Game_KC_H              SDL_SCANCODE_H
# define Game_KC_I              SDL_SCANCODE_I
# define Game_KC_J              SDL_SCANCODE_J
# define Game_KC_K              SDL_SCANCODE_K
# define Game_KC_L              SDL_SCANCODE_L
# define Game_KC_M              SDL_SCANCODE_M
# define Game_KC_N              SDL_SCANCODE_N
# define Game_KC_O              SDL_SCANCODE_O
# define Game_KC_P              SDL_SCANCODE_P
# define Game_KC_Q              SDL_SCANCODE_Q
# define Game_KC_R              SDL_SCANCODE_R
# define Game_KC_S              SDL_SCANCODE_S
# define Game_KC_T              SDL_SCANCODE_T
# define Game_KC_U              SDL_SCANCODE_U
# define Game_KC_V              SDL_SCANCODE_V
# define Game_KC_W              SDL_SCANCODE_W
# define Game_KC_X              SDL_SCANCODE_X
# define Game_KC_Y              SDL_SCANCODE_Y
# define Game_KC_Z              SDL_SCANCODE_Z
# define Game_KC_1              SDL_SCANCODE_1
# define Game_KC_2              SDL_SCANCODE_2
# define Game_KC_3              SDL_SCANCODE_3
# define Game_KC_4              SDL_SCANCODE_4
# define Game_KC_5              SDL_SCANCODE_5
# define Game_KC_6              SDL_SCANCODE_6
# define Game_KC_7              SDL_SCANCODE_7
# define Game_KC_8              SDL_SCANCODE_8
# define Game_KC_9              SDL_SCANCODE_9
# define Game_KC_0              SDL_SCANCODE_0
# define Game_KC_RETURN         SDL_SCANCODE_RETURN
# define Game_KC_ESCAPE         SDL_SCANCODE_ESCAPE
# define Game_KC_BACKSPACE      SDL_SCANCODE_BACKSPACE
# define Game_KC_TAB            SDL_SCANCODE_TAB
# define Game_KC_SPACE          SDL_SCANCODE_SPACE
# define Game_KC_COMMA          SDL_SCANCODE_COMMA
# define Game_KC_PERIOD         SDL_SCANCODE_PERIOD
# define Game_KC_SLASH          SDL_SCANCODE_SLASH
# define Game_KC_CAPSLOCK       SDL_SCANCODE_CAPSLOCK
# define Game_KC_F1             SDL_SCANCODE_F1
# define Game_KC_F2             SDL_SCANCODE_F2
# define Game_KC_F3             SDL_SCANCODE_F3
# define Game_KC_F4             SDL_SCANCODE_F4
# define Game_KC_F5             SDL_SCANCODE_F5
# define Game_KC_F6             SDL_SCANCODE_F6
# define Game_KC_F7             SDL_SCANCODE_F7
# define Game_KC_F8             SDL_SCANCODE_F8
# define Game_KC_F9             SDL_SCANCODE_F9
# define Game_KC_F10            SDL_SCANCODE_F10
# define Game_KC_F11            SDL_SCANCODE_F11
# define Game_KC_F12            SDL_SCANCODE_F12
# define Game_KC_PRINTSCREEN    SDL_SCANCODE_PRINTSCREEN
# define Game_KC_SCROLLLOCK     SDL_SCANCODE_SCROLLLOCK
# define Game_KC_PAUSE          SDL_SCANCODE_PAUSE
# define Game_KC_INSERT         SDL_SCANCODE_INSERT
# define Game_KC_HOME           SDL_SCANCODE_HOME
# define Game_KC_PAGEUP         SDL_SCANCODE_PAGEUP
# define Game_KC_DELETE         SDL_SCANCODE_DELETE
# define Game_KC_END            SDL_SCANCODE_END
# define Game_KC_PAGEDOWN       SDL_SCANCODE_PAGEDOWN
# define Game_KC_RIGHT          SDL_SCANCODE_RIGHT
# define Game_KC_LEFT           SDL_SCANCODE_LEFT
# define Game_KC_DOWN           SDL_SCANCODE_DOWN
# define Game_KC_UP             SDL_SCANCODE_UP
#else
#error TODO
#endif

