
typedef union palcnvmap {
    uint16_t                    map16[256];
    uint32_t                    map32[256];
} palcnvmap;

void Game_BeginPaletteUpdate(unsigned char idx);
void Game_SetPaletteEntry(unsigned char r,unsigned char g,unsigned char b);
void Game_FinishPaletteUpdates(void);

