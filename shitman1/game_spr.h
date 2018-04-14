
#define Game_SpriteMax                  128

#define Game_SS_Flag_Enabled            (1U << 0U)      /* sprite is enabled (visible) */
#define Game_SS_Flag_Transparent        (1U << 1U)      /* transparent (color 0 is transparent) */
#define Game_SS_Flag_Allocated          (1U << 2U)      /* sprite is allocated */
#define Game_SS_Flag_Dirty              (1U << 3U)      /* sprite needs redraw */

typedef struct game_sprite_t {
    int16_t                 x,y;
    uint16_t                w,h;
    uint16_t                flags;
    uint16_t                stride;
#if TARGET_MSDOS == 16
    unsigned char far*      ptr;
#else
    unsigned char*          ptr;
#endif
} game_sprite_t;

typedef uint16_t            game_sprite_index_t;

static const uint16_t       game_sprite_index_none = 0xFFFFU;

game_sprite_t *Game_SpriteSlot(const game_sprite_index_t i);
void Game_SpriteDirtySlot(game_sprite_index_t r);
void Game_SpriteSlotMove(const game_sprite_index_t i,const int16_t x,const int16_t y);
game_sprite_index_t Game_SpriteAllocSlotSearch(const game_sprite_index_t max1/*stop just before this index*/);
void memcpy_transparent(unsigned char *d,const unsigned char *s,unsigned int sz);
void Game_SpriteDirtySlot(game_sprite_index_t r);

#if TARGET_MSDOS == 16
void Game_SpriteSlotSetImage(const game_sprite_index_t i,const uint16_t w,const uint16_t h,const uint16_t stride,unsigned char far *const ptr,const uint16_t flags);
#else
void Game_SpriteSlotSetImage(const game_sprite_index_t i,const uint16_t w,const uint16_t h,const uint16_t stride,unsigned char *const ptr,const uint16_t flags);
#endif

void Game_SpriteDrawBltItem(game_sprite_t * const slot);
void Game_SpriteDrawBlt(void);
void Game_SpriteDraw(void);
void Game_SpriteSlotShow(game_sprite_index_t r);
void Game_SpriteSlotHide(game_sprite_index_t r);
void Game_SpriteFreeSlot(game_sprite_index_t r);
game_sprite_index_t Game_SpriteAllocSlot(void);
void Game_SpriteClearAll(void);
int Game_SpriteInit(void);
void Game_SpriteShutdown(void);

