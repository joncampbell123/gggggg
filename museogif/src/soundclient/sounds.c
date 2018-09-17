

/* sounds.c */

#include "doomdef.h"
#include "sounds.h"

/* Music info */

musicinfo_t S_music[] =
{
  { 0, 0, 0, 0 },
  { "MUS_E1M1", 0, NULL, 0 }, /* 1-1 */
  { "MUS_E1M2", 0, NULL, 0 },
  { "MUS_E1M3", 0, NULL, 0 },
  { "MUS_E1M4", 0, NULL, 0 },
  { "MUS_E1M5", 0, NULL, 0 },
  { "MUS_E1M6", 0, NULL, 0 },
  { "MUS_E1M7", 0, NULL, 0 },
  { "MUS_E1M8", 0, NULL, 0 },
  { "MUS_E1M9", 0, NULL, 0 },

  { "MUS_E2M1", 0, NULL, 0 }, /* 2-1 */
  { "MUS_E2M2", 0, NULL, 0 },
  { "MUS_E2M3", 0, NULL, 0 },
  { "MUS_E2M4", 0, NULL, 0 },
  { "MUS_E1M4", 0, NULL, 0 },
  { "MUS_E2M6", 0, NULL, 0 },
  { "MUS_E2M7", 0, NULL, 0 },
  { "MUS_E2M8", 0, NULL, 0 },
  { "MUS_E2M9", 0, NULL, 0 },

  { "MUS_E1M1", 0, NULL, 0 }, /* 3-1 */
  { "MUS_E3M2", 0, NULL, 0 },
  { "MUS_E3M3", 0, NULL, 0 },
  { "MUS_E1M6", 0, NULL, 0 },
  { "MUS_E1M3", 0, NULL, 0 },
  { "MUS_E1M2", 0, NULL, 0 },
  { "MUS_E1M5", 0, NULL, 0 },
  { "MUS_E1M9", 0, NULL, 0 },
  { "MUS_E2M6", 0, NULL, 0 },

  { "MUS_E1M6", 0, NULL, 0 }, /* 4-1 */
  { "MUS_E1M2", 0, NULL, 0 },
  { "MUS_E1M3", 0, NULL, 0 },
  { "MUS_E1M4", 0, NULL, 0 },
  { "MUS_E1M5", 0, NULL, 0 },
  { "MUS_E1M1", 0, NULL, 0 },
  { "MUS_E1M7", 0, NULL, 0 },
  { "MUS_E1M8", 0, NULL, 0 },
  { "MUS_E1M9", 0, NULL, 0 },

  { "MUS_E2M1", 0, NULL, 0 }, /* 5-1 */
  { "MUS_E2M2", 0, NULL, 0 },
  { "MUS_E2M3", 0, NULL, 0 },
  { "MUS_E2M4", 0, NULL, 0 },
  { "MUS_E1M4", 0, NULL, 0 },
  { "MUS_E2M6", 0, NULL, 0 },
  { "MUS_E2M7", 0, NULL, 0 },
  { "MUS_E2M8", 0, NULL, 0 },
  { "MUS_E2M9", 0, NULL, 0 },

  { "MUS_E3M2", 0, NULL, 0 }, /* 6-1 */
  { "MUS_E3M3", 0, NULL, 0 }, /* 6-2 */
  { "MUS_E1M6", 0, NULL, 0 }, /* 6-3 */

  { "MUS_TITL", 0, NULL, 0 },
  { "MUS_INTR", 0, NULL, 0 },
  { "MUS_CPTD", 0, NULL, 0 }
};


/* Sound info */

sfxinfo_t S_sfx[] =
{
  { "none", NULL, 0, NULL, -1, -1, false, -1, 0, 0 },
  { "gldhit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "gntful", NULL, 32, NULL, -1, -1, false, -1, 0, -1 },
  { "gnthit", NULL, 32, NULL, -1, -1, false, -1, 0, -1 },
  { "gntpow", NULL, 32, NULL, -1, -1, false, -1, 0, -1 },
  { "gntact", NULL, 32, NULL, -1, -1, true, -1, 0, -1 },
  { "gntuse", NULL, 32, NULL, -1, -1, false, -1, 0, -1 },
  { "phosht", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "phohit", NULL, 32, NULL, -1, -1, false, -1, 0, -1 },
  { "-phopow", &S_sfx[sfx_hedat1], 32, NULL, -1, -1, false, -1, 0, 1 },
  { "lobsht", NULL, 20, NULL, -1, -1, false, -1, 0, 2 },
  { "lobhit", NULL, 20, NULL, -1, -1, false, -1, 0, 2 },
  { "lobpow", NULL, 20, NULL, -1, -1, false, -1, 0, 2 },
  { "hrnsht", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "hrnhit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "hrnpow", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "ramphit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "ramrain", NULL, 10, NULL, -1, -1, false, -1, 0, 2 },
  { "bowsht", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "stfhit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "stfpow", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "stfcrk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "impsit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "impat1", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "impat2", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "impdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "-impact", &S_sfx[sfx_impsit], 20, NULL, -1, -1, true, -1, 0, 2 },
  { "imppai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "mumsit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "mumat1", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "mumat2", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "mumdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "-mumact", &S_sfx[sfx_mumsit], 20, NULL, -1, -1, true, -1, 0, 2 },
  { "mumpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "mumhed", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "bstsit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "bstatk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "bstdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "bstact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "bstpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "clksit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "clkatk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "clkdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "clkact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "clkpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "snksit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "snkatk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "snkdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "snkact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "snkpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "kgtsit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "kgtatk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "kgtat2", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "kgtdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "-kgtact", &S_sfx[sfx_kgtsit], 20, NULL, -1, -1, true, -1, 0, 2 },
  { "kgtpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "wizsit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "wizatk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "wizdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "wizact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "wizpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "minsit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "minat1", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "minat2", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "minat3", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "mindth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "minact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "minpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "hedsit", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "hedat1", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "hedat2", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "hedat3", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "heddth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "hedact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "hedpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "sorzap", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "sorrise", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "sorsit", NULL, 200, NULL, -1, -1, true, -1, 0, 2 },
  { "soratk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "soract", NULL, 200, NULL, -1, -1, true, -1, 0, 2 },
  { "sorpai", NULL, 200, NULL, -1, -1, false, -1, 0, 2 },
  { "sordsph", NULL, 200, NULL, -1, -1, false, -1, 0, 2 },
  { "sordexp", NULL, 200, NULL, -1, -1, false, -1, 0, 2 },
  { "sordbon", NULL, 200, NULL, -1, -1, false, -1, 0, 2 },
  { "-sbtsit", &S_sfx[sfx_bstsit], 32, NULL, -1, -1, true, -1, 0, 2 },
  { "-sbtatk", &S_sfx[sfx_bstatk], 32, NULL, -1, -1, false, -1, 0, 2 }, 
  { "sbtdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "sbtact", NULL, 20, NULL, -1, -1, true, -1, 0, 2 },
  { "sbtpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "plroof", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "plrpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "plrdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "gibdth", NULL, 100, NULL, -1, -1, false, -1, 0, 2 },
  { "plrwdth", NULL, 80, NULL, -1, -1, false, -1, 0, 2 },
  { "plrcdth", NULL, 100, NULL, -1, -1, false, -1, 0, 2 },
  { "itemup", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "wpnup", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "telept", NULL, 50, NULL, -1, -1, false, -1, 0, 2 },
  { "doropn", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "dorcls", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "dormov", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "artiup", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "switch", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "pstart", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "pstop", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "stnmov", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "chicpai", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "chicatk", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "chicdth", NULL, 40, NULL, -1, -1, false, -1, 0, 2 },
  { "chicact", NULL, 32, NULL, -1, -1, true, -1, 0, 2 },
  { "chicpk1", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "chicpk2", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "chicpk3", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "keyup", NULL, 50, NULL, -1, -1, true, -1, 0, 2 },
  { "ripslop", NULL, 16, NULL, -1, -1, false, -1, 0, 2 },
  { "newpod", NULL, 16, NULL, -1, -1, false, -1, 0, -1 },
  { "podexp", NULL, 40, NULL, -1, -1, false, -1, 0, -1 },
  { "bounce", NULL, 16, NULL, -1, -1, false, -1, 0, 2 },
  { "-volsht", &S_sfx[sfx_bstatk], 16, NULL, -1, -1, false, -1, 0, 2 },
  { "-volhit", &S_sfx[sfx_lobhit], 16, NULL, -1, -1, false, -1, 0, 2 },
  { "burn", NULL, 10, NULL, -1, -1, false, -1, 0, 2 },
  { "splash", NULL, 10, NULL, -1, -1, false, -1, 0, 1 },
  { "gloop", NULL, 10, NULL, -1, -1, false, -1, 0, 2 },
  { "respawn", NULL, 10, NULL, -1, -1, false, -1, 0, 1 },
  { "blssht", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "blshit", NULL, 32, NULL, -1, -1, false, -1, 0, 2 },
  { "chat", NULL, 100, NULL, -1, -1, false, -1, 0, 1 },
  { "artiuse", NULL, 32, NULL, -1, -1, false, -1, 0, 1 },
  { "gfrag", NULL, 100, NULL, -1, -1, false, -1, 0, 1 },
  { "waterfl", NULL, 16, NULL, -1, -1, false, -1, 0, 2 },
  
  /* Monophonic sounds */
  
  { "wind", NULL, 16, NULL, -1, -1, false, -1, 0, 1 },
  { "amb1", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb2", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb3", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb4", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb5", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb6", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb7", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb8", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb9", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb10", NULL, 1, NULL, -1, -1, false, -1, 0, 1 },
  { "amb11", NULL, 1, NULL, -1, -1, false, -1, 0, 0 }
};
