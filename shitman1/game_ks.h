
typedef uint8_t                 Game_KeyState;

#define Game_KS_DOWN            (1U << 0U)
#define Game_KS_LCTRL           (1U << 1U)
#define Game_KS_RCTRL           (1U << 2U)
#define Game_KS_LALT            (1U << 3U)
#define Game_KS_RALT            (1U << 4U)
#define Game_KS_LSHIFT          (1U << 5U)
#define Game_KS_RSHIFT          (1U << 6U)

#define Game_KS_CTRL            (Game_KS_LCTRL  | Game_KS_RCTRL)
#define Game_KS_ALT             (Game_KS_LALT   | Game_KS_RALT)
#define Game_KS_SHIFT           (Game_KS_LSHIFT | Game_KS_RSHIFT)

void Game_KeyShiftStateSet(unsigned int f,unsigned char s);

extern unsigned int             Game_KeyShiftState;

