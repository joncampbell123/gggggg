
/* V_video.c */

#include "doomdef.h"

static inline int chps( byte );

byte *screen;
int dirtybox[4];
long usegamma;

int screenwidth,screenheight;
int maxblocks,minblocks;

byte gammatable[5][256] =
{
{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255},

{2,4,5,7,8,10,11,12,14,15,16,18,19,20,21,23,24,25,26,27,29,30,31,32,33,34,36,37,38,39,40,41,42,44,45,46,47,48,49,50,51,52,54,55,56,57,58,59,60,61,62,63,64,65,66,67,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,163,164,165,166,167,168,169,170,171,172,173,174,175,175,176,177,178,179,180,181,182,183,184,185,186,186,187,188,189,190,191,192,193,194,195,196,196,197,198,199,200,201,202,203,204,205,205,206,207,208,209,210,211,212,213,214,214,215,216,217,218,219,220,221,222,222,223,224,225,226,227,228,229,230,230,231,232,233,234,235,236,237,237,238,239,240,241,242,243,244,245,245,246,247,248,249,250,251,252,252,253,254,255},

{4,7,9,11,13,15,17,19,21,22,24,26,27,29,30,32,33,35,36,38,39,40,42,43,45,46,47,48,50,51,52,54,55,56,57,59,60,61,62,63,65,66,67,68,69,70,72,73,74,75,76,77,78,79,80,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,133,134,135,136,137,138,139,140,141,142,143,144,144,145,146,147,148,149,150,151,152,153,153,154,155,156,157,158,159,160,160,161,162,163,164,165,166,166,167,168,169,170,171,172,172,173,174,175,176,177,178,178,179,180,181,182,183,183,184,185,186,187,188,188,189,190,191,192,193,193,194,195,196,197,197,198,199,200,201,201,202,203,204,205,206,206,207,208,209,210,210,211,212,213,213,214,215,216,217,217,218,219,220,221,221,222,223,224,224,225,226,227,228,228,229,230,231,231,232,233,234,235,235,236,237,238,238,239,240,241,241,242,243,244,244,245,246,247,247,248,249,250,251,251,252,253,254,254,255},

{8,12,16,19,22,24,27,29,31,34,36,38,40,41,43,45,47,49,50,52,53,55,57,58,60,61,63,64,65,67,68,70,71,72,74,75,76,77,79,80,81,82,84,85,86,87,88,90,91,92,93,94,95,96,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,135,136,137,138,139,140,141,142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,155,156,157,158,159,160,160,161,162,163,164,165,165,166,167,168,169,169,170,171,172,173,173,174,175,176,176,177,178,179,180,180,181,182,183,183,184,185,186,186,187,188,189,189,190,191,192,192,193,194,195,195,196,197,197,198,199,200,200,201,202,202,203,204,205,205,206,207,207,208,209,210,210,211,212,212,213,214,214,215,216,216,217,218,219,219,220,221,221,222,223,223,224,225,225,226,227,227,228,229,229,230,231,231,232,233,233,234,235,235,236,237,237,238,238,239,240,240,241,242,242,243,244,244,245,246,246,247,247,248,249,249,250,251,251,252,253,253,254,254,255},

{16,23,28,32,36,39,42,45,48,50,53,55,57,60,62,64,66,68,69,71,73,75,76,78,80,81,83,84,86,87,89,90,92,93,94,96,97,98,100,101,102,103,105,106,107,108,109,110,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,128,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,155,156,157,158,159,159,160,161,162,163,163,164,165,166,166,167,168,169,169,170,171,172,172,173,174,175,175,176,177,177,178,179,180,180,181,182,182,183,184,184,185,186,187,187,188,189,189,190,191,191,192,193,193,194,195,195,196,196,197,198,198,199,200,200,201,202,202,203,203,204,205,205,206,207,207,208,208,209,210,210,211,211,212,213,213,214,214,215,216,216,217,217,218,219,219,220,220,221,221,222,223,223,224,224,225,225,226,227,227,228,228,229,229,230,230,231,232,232,233,233,234,234,235,235,236,236,237,237,238,239,239,240,240,241,241,242,242,243,243,244,244,245,245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,253,254,254,255,255}
};



/*
 * V_MarkRect
 */
void V_MarkRect( 
		int		x,
		int		y,
		int		width,
		int		height )
{
  M_AddToBox (dirtybox, x, y);
  M_AddToBox (dirtybox, x+width-1, y+height-1);
}


/*
 * V_CopyRect
 */

/* needed ???
void V_CopyRect( 
		int		srcx,
		int		srcy,
		int		srcscrn,
		int		width,
		int		height,
		int		destx,
		int		desty,
		int		destscrn )
{
  byte*	src;
  byte*	dest;
  
#ifdef RANGECHECK
  if (srcx<0
      || srcx+width >screenwidth
      || srcy<0
      || srcy+height>screenheight
      || destx<0||destx+width >screenwidth
      || desty<0
      || desty+height>screenheight
      || (unsigned)srcscrn>4
      || (unsigned)destscrn>4)
    {
      I_Error ("Bad V_CopyRect");
    }
#endif
  V_MarkRect (destx, desty, width, height);
  
  src = screen+screenwidth*srcy+srcx;
  dest = screen+screenwidth*desty+destx;
  
  for ( ; height>0 ; height--)
    {
      memcpy (dest, src, width);
      src += screenwidth;
      dest += screenwidth;
    }
}
*/

/*
 * ---------------------------------------------------------------------------
 * 
 *  PROC V_DrawPatch
 * 
 *  Draws a column based masked pic to the screen.
 * 
 * ---------------------------------------------------------------------------
 */

void V_DrawPatch(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  byte *desttop;
  byte *dest;
  byte *source;
  int w;
  
  /* Center Patch */
  x += X_DISP;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);
	
#ifdef RANGECHECK
  if(x < 0 
     || x+SHORT(patch->width) > screenwidth
     || y < 0
     || y+SHORT(patch->height) > screenheight)
    {
      I_Error("Bad V_DrawPatch");
    }
#endif

  /* not needed: no doublebuffering !!!
     if (!scrn)
     V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));
  */
  
  col = 0;
  desttop = screen+y*screenwidth+x;
  w = SHORT(patch->width);
  for(; col < w; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch+LONG(patch->columnofs[col]));
      /* Step through the posts in a column */
      while(column->topdelta != 0xff)
	{
	  source = (byte *)column+3;
	  dest = desttop+column->topdelta*screenwidth;
	  count = column->length;
	  while(count--)
	    {
	      *dest = *source++;
	      dest += screenwidth;
	    }
	  column = (column_t *)((byte *)column+column->length+4);
	}
    }
}

/*
  ==================
  =
  = V_DrawFuzzPatch
  =
  = Masks a column based translucent masked pic to the screen.
  =
  ==================
*/
extern byte *tinttable;

void V_DrawFuzzPatch (int x, int y, patch_t *patch)
{
  int		count,col;
  column_t	*column;
  byte		*desttop, *dest, *source;
  int		w;
	
  /* Center Patch */
  x += X_DISP;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);
  
#ifdef RANGECHECK
  if (x<0
      ||x+SHORT(patch->width) >screenwidth 
      || y<0 
      || y+SHORT(patch->height)>screenheight)
    {
      I_Error ("Bad V_DrawFuzzPatch");
    }
#endif
  col = 0;
  desttop = screen+y*screenwidth+x;
  
  w = SHORT(patch->width);
  for ( ; col<w ; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
      
      /* step through the posts in a column */
      
      while (column->topdelta != 0xff )
	{
	  source = (byte *)column + 3;
	  dest = desttop + column->topdelta*screenwidth;
	  count = column->length;
	  
	  while (count--)
	    {
	      *dest = tinttable[((*dest)<<8) + *source++];
	      dest += screenwidth;
	    }
	  column = (column_t *)(  (byte *)column + column->length + 4 );
	}
    }			
}

/*
  ==================
  =
  = V_DrawShadowedPatch
  =
  = Masks a column based masked pic to the screen.
  =
  ==================
*/

void V_DrawShadowedPatch(int x, int y, patch_t *patch)
{
  int		count,col;
  column_t	*column;
  byte		*desttop, *dest, *source;
  byte		*desttop2, *dest2;
  int		w;
  
  /* Center Patch */
  x += X_DISP;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK  
  if (x<0
      ||x+SHORT(patch->width) >screenwidth 
      || y<0 || y+SHORT(patch->height)>screenheight)
    {
      I_Error ("Bad V_DrawShadowedPatch");
    }
#endif
  
  col = 0;
  desttop = screen+y*screenwidth+x;
  desttop2 = screen+(y+2)*screenwidth+x+2;
  
  w = SHORT(patch->width);
  for ( ; col<w ; x++, col++, desttop++, desttop2++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      /* step through the posts in a column */
      
      while (column->topdelta != 0xff )
	{
	  source = (byte *)column + 3;
	  dest = desttop + column->topdelta*screenwidth;
	  dest2 = desttop2 + column->topdelta*screenwidth;
	  count = column->length;
	  
	  while (count--)
	    {
	      *dest2 = tinttable[((*dest2)<<8)];
	      dest2 += screenwidth;
	      *dest = *source++;
	      dest += screenwidth;
	      
	    }
	  column = (column_t *)(  (byte *)column + column->length + 4 );
	}
    }			
}

/*
 * ---------------------------------------------------------------------------
 * 
 *  PROC V_DrawRawScreen
 * 
 * ---------------------------------------------------------------------------
 */

/*
  Routine to do 2x scaling
*/

/* FIXME: depends on unsigned int being 32 bits */
typedef unsigned int scaleu32;

static inline void
do_scale2x(int stride, byte *dest, byte *src)
{
#define xsize 320
#define ysize 200
	int i, j, destinc = stride*2-xsize*2;
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; /* i is incremented below */) {
			register scaleu32 pix1 = src[i++], pix2 = src[i++];
#ifndef __BIG_ENDIAN__
			*((scaleu32 *) (dest + stride))
				= *((scaleu32 *) dest)
				= (pix1 | (pix1 << 8)
				   | (pix2 << 16) | (pix2 << 24));
#else
			*((scaleu32 *) (dest + stride))
				= *((scaleu32 *) dest)
				= (pix2 | (pix2 << 8)
				   | (pix1 << 16) | (pix1 << 24));
#endif
			dest += 4;
		}
		dest += destinc;
		src += xsize;
	}
}

void V_DrawRawScreen(byte *raw)
{
	if (screenwidth != SCREENWIDTH || SCREENHEIGHT != screenheight) {
		byte *scrptr;
		int y;

		/* Blank border first */
		memset(screen, 0, screenwidth*screenheight);

		if (screenwidth >= SCREENWIDTH*2 &&
		    screenheight >= SCREENHEIGHT*2) {
			/* Scale up the screen */
			scrptr = screen + (screenwidth-SCREENWIDTH*2)/2 +
				((screenheight-SCREENHEIGHT*2)/2)*screenwidth;
			do_scale2x(screenwidth, scrptr, raw);
			return;
		}

		scrptr = screen + X_DISP + Y_DISP*screenwidth;
		for (y = 0; y < SCREENHEIGHT; y++) {
			memcpy(scrptr, raw, SCREENWIDTH);
			scrptr += screenwidth;
			raw += SCREENWIDTH;
		}
	} else {
		memcpy(screen, raw, SCREENWIDTH*SCREENHEIGHT);
	}
}

/*
 * ---------------------------------------------------------------------------
 * 
 *  PROC V_Init
 * 
 * ---------------------------------------------------------------------------
 */

void V_Init(void)
{
  int res=M_CheckParm("-mode");
  if (res)
    {
      if (res+2>=myargc) I_Error("Not enough parameters, example: "
				 "-mode 320 200\n");
      screenwidth=atoi(myargv[res+1]);
      screenheight=atoi(myargv[res+2]);
      
      if (screenwidth<320 || screenheight<200)
	{
	  I_Error("Resolution too low!\n");
	}
      if (screenwidth>MAXSCREENWIDTH || screenheight>MAXSCREENHEIGHT)
	{
	  I_Error("Resolution too high!\n"
		  "Please change MAXSCREEN* in doomdef.h\n");
	}
    }
  else
    {
      screenwidth=SCREENWIDTH;
      screenheight=SCREENHEIGHT;
    }
  
  I_CheckRes();  /* check for valid resolutions */

  maxblocks=screenwidth/32+1;
  minblocks=3;
  if ((minblocks*(screenheight-SBARHEIGHT)/(maxblocks-1)) < 40) {
	  minblocks++;
  }
  
#ifndef UNIX
  /* I_AllocLow will put screen in low dos memory on PCs. */
  screen = I_AllocLow(screenwidth*screenheight);
#endif
}


/*
 * Function for bilinear filtering of screen[]
 * There is much todo, because the palatte entries arn't ordered linear !
 * Here only the pixels mathcing the first 32 graycolor palette 
 * entries are filtered !!!!!!
 */


static inline int chps(byte indexp)
{
    if (indexp <= 32) 
	return 1;
	    else 
	return 0;
}

void V_Filter_Screen_linear(byte* screenp)
{
    unsigned int i;
    int scrx=screenwidth;
    int scry=screenheight;

    /* kind of filtering:

     * x *

     */
    
    for ( i=0; i<(unsigned)(scrx*scry); i++ )
	{
	    if (chps(screenp[i-1]) && chps(screenp[i]) && chps(screenp[i+1]))
		{
		    screenp[i]=(byte) ((screenp[i-1]+screenp[i]+screenp[i+1])/3);
		}
	}
}

void V_Filter_Screen_bilinear(byte* screenp)
{
    unsigned int i;
    int scrx=screenwidth;
    int scry=screenheight;
    
    /* kind of filtering:

     * * *
     * x *
     * * *

     */


    for ( i=0; i<(unsigned)(scrx*scry); i++ )
	{
	    if (chps(screenp[i-1]) && chps(screenp[i]) && chps(screenp[i+1]) &&
		chps(screenp[i-scrx]) && chps(screenp[i+scrx]) &&
		chps(screenp[i-scrx-1]) && chps(screenp[i-scrx+1]) &&
		chps(screenp[i+scrx-1]) && chps(screenp[i+scrx+1]))
		{
		    screenp[i]=(byte) ((screenp[i-1]+screenp[i]+screenp[i+1]+
					screenp[i-scrx]+screenp[i+scrx]+
					screenp[i-scrx-1]+screenp[i-scrx+1]+
					screenp[i+scrx-1]+screenp[i+scrx+1])/9);
		}
	}
}

