/* R_draw.c */

#include "doomdef.h"
#include "r_local.h"

/*
  
  All drawing to the view buffer is accomplished in this file.  The other refresh
  files only know about ccordinates, not the architecture of the frame buffer.
  
*/

byte *viewimage;
int viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;
byte *ylookup[MAXSCREENHEIGHT];
int columnofs[MAXSCREENWIDTH];
byte translations[3][256];     /* color tables for different players */
byte *tinttable;               /* used for translucent sprites */

/*
  ==================
  =
  = R_DrawColumn
  =
  = Source is the top of the column to scale
  =
  ==================
*/

lighttable_t	*dc_colormap;
int		dc_x;
int		dc_yl;
int		dc_yh;
fixed_t		dc_iscale;
fixed_t		dc_texturemid;
byte		*dc_source;	/* first pixel in a column (possibly virtual) */

int		dccount;        /* just for profiling */


void R_DrawColumn (void)
{
  int		count;
  byte		*dest;
  fixed_t	frac, fracstep;	
  
  count = dc_yh - dc_yl;
  if (count < 0)
    return;
				
#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)screenwidth 
      || dc_yl < 0 
      || (unsigned)dc_yh >= (unsigned)screenheight)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif
  
  dest = ylookup[dc_yl] + columnofs[dc_x]; 
  
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;
  
  do
    {
      *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
      dest += screenwidth;
      frac += fracstep;
    } while (count--);
}


void R_DrawColumnLow (void)
{
  int		count;
  byte		*dest;
  fixed_t	frac, fracstep;	
  
  count = dc_yh - dc_yl;
  if (count < 0)
    return;
  
#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)screenwidth 
      || dc_yl < 0 
      || (unsigned)dc_yh >= (unsigned)screenheight)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
  /*	dccount++; */
#endif
  
  dest = ylookup[dc_yl] + columnofs[dc_x]; 
  
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;
  
  do
    {
      *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
      dest += screenwidth;
      frac += fracstep;
    } while (count--);
}


/*
 * Spectre/Invisibility.
 */
#define FUZZTABLE	50

#define FUZZOFF	  (1)  /* could be 1 */

int fuzzoffset[FUZZTABLE] = {
  FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};
int fuzzpos = 0;


/*
 * Initialize fuzzoffset[] table to current screen size
 */
void R_InitFuzzTable(void)
{
  int i;
  
  for (i=0; i<FUZZTABLE; i++) fuzzoffset[i] *= screenwidth;
}

/*
 * Framebuffer postprocessing.
 * Creates a fuzzy image by copying pixels
 *  from adjacent ones to left and right.
 * Used with an all black colormap, this
 *  could create the SHADOW effect,
 *  i.e. spectres and invisible players.
 */
void R_DrawFuzzColumn (void)
{
  int		count;
  byte		*dest;
  fixed_t	frac, fracstep;	
  
  if (!dc_yl)
    dc_yl = 1;
  if (dc_yh == viewheight-1)
    dc_yh = viewheight - 2;
  
  count = dc_yh - dc_yl;
  if (count < 0)
    return;
  
#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)screenwidth 
      || dc_yl < 0 
      || (unsigned)dc_yh >= (unsigned)screenheight)
    I_Error ("R_DrawFuzzColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif
  
  dest = ylookup[dc_yl] + columnofs[dc_x];
  
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;
  
  do
    {
      *dest = tinttable[((*dest)<<8)+dc_colormap[dc_source[(frac>>FRACBITS)&127]]];
      
      dest += screenwidth;
      frac += fracstep;
    } while(count--);
}

/*
  ========================
  =
  = R_DrawTranslatedColumn
  =
  ========================
*/

byte *dc_translation;
byte *translationtables;

void R_DrawTranslatedColumn (void)
{
  int		count;
  byte		*dest;
  fixed_t       frac, fracstep;	
  
  count = dc_yh - dc_yl;
  if (count < 0)
    return;
  
#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)screenwidth 
      || dc_yl < 0 
      || (unsigned)dc_yh >= (unsigned)screenheight)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif
  
  dest = ylookup[dc_yl] + columnofs[dc_x];
  
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;
  
  do
    {
      *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
      dest += screenwidth;
      frac += fracstep;
    } while (count--);
}

void R_DrawTranslatedFuzzColumn (void)
{
  int		count;
  byte		*dest;
  fixed_t	frac, fracstep;	
  
  count = dc_yh - dc_yl;
  if (count < 0)
    return;
  
#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)screenwidth 
      || dc_yl < 0 
      || (unsigned)dc_yh >= (unsigned)screenheight)
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif
  
  dest = ylookup[dc_yl] + columnofs[dc_x];
  
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;
  
  do
    {
      *dest = tinttable[((*dest)<<8)
		       +dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]];
      dest += screenwidth;
      frac += fracstep;
    } while (count--);
}

/*
 * --------------------------------------------------------------------------
 * 
 *  PROC R_InitTranslationTables
 * 
 * --------------------------------------------------------------------------
 */

void R_InitTranslationTables (void)
{
  int i;

  /* Load tint table */
  tinttable = W_CacheLumpName("TINTTAB", PU_STATIC);
  
  /* Allocate translation tables */
  translationtables = Z_Malloc(256*3+255, PU_STATIC, 0);
  translationtables = (byte *)(((long)translationtables + 255) & ~255);
  
  /* Fill out the translation tables */
  for(i = 0; i < 256; i++)
    {
      if(i >= 225 && i <= 240)
	{
	  translationtables[i] = 114+(i-225);      /* yellow */
	  translationtables[i+256] = 145+(i-225);  /* red */
	  translationtables[i+512] = 190+(i-225);  /* blue */
	}
      else
	{
	  translationtables[i] = translationtables[i+256] 
	    = translationtables[i+512] = i;
	}
    }
}

/*
  ================
  =
  = R_DrawSpan
  =
  ================
*/

int			ds_y;
int			ds_x1;
int			ds_x2;
lighttable_t	        *ds_colormap;
fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;
byte			*ds_source;		/* start of a 64*64 tile image */

int			dscount;		/* just for profiling */


void R_DrawSpan (void)
{
  fixed_t	xfrac, yfrac;
  byte		*dest;
  int	        count, spot;
  
#ifdef RANGECHECK
  if (ds_x2 < ds_x1 
      || ds_x1<0 
      || ds_x2>=screenwidth 
      || (unsigned)ds_y>(unsigned)screenheight)
    I_Error ("R_DrawSpan: %i to %i at %i",ds_x1,ds_x2,ds_y);
  //	dscount++;
#endif
  
  xfrac = ds_xfrac;
  yfrac = ds_yfrac;
  
  dest = ylookup[ds_y] + columnofs[ds_x1];	
  count = ds_x2 - ds_x1;
  do
    {
      spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
      *dest++ = ds_colormap[ds_source[spot]];
      xfrac += ds_xstep;
      yfrac += ds_ystep;
    } while (count--);
}


void R_DrawSpanLow (void)
{
  fixed_t	xfrac, yfrac;
  byte		*dest;
  int		count, spot;
  
#ifdef RANGECHECK
  if (ds_x2 < ds_x1 
      || ds_x1<0 
      || ds_x2>=screenwidth
      || (unsigned)ds_y>(unsigned)screenheight)
    I_Error ("R_DrawSpan: %i to %i at %i",ds_x1,ds_x2,ds_y);
  //	dscount++;
#endif
  
  xfrac = ds_xfrac;
  yfrac = ds_yfrac;
  
  dest = ylookup[ds_y] + columnofs[ds_x1];	
  count = ds_x2 - ds_x1;
  do
    {
      spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
      *dest++ = ds_colormap[ds_source[spot]];
      xfrac += ds_xstep;
      yfrac += ds_ystep;
    } while (count--);
}



/*
  ================
  =
  = R_InitBuffer
  =
  =================
*/

void R_InitBuffer (int width, int height)
{
  int		i;
  
  viewwindowx = (screenwidth-width) >> 1;
  for (i=0 ; i<width ; i++)
    columnofs[i] = viewwindowx + i;
  if (width == screenwidth)
    viewwindowy = 0;
  else
    viewwindowy = (screenheight-SBARHEIGHT-height) >> 1;
  for (i=0 ; i<height ; i++)
    ylookup[i] = screen + (i+viewwindowy)*screenwidth;
}


/*
  In R_DrawViewBorder() and R_DrawTopBorder() coordinates are already
  correct, so we have to subtract X_DISP from the x-coordinate to
  compensate for the addition in V_DrawPatch().
*/

/*
  ==================
  =
  = R_DrawViewBorder
  =
  = Draws the border around the view for different size windows
  ==================
*/

boolean BorderNeedRefresh;

void R_DrawViewBorder (void)
{
  byte	        *src, *dest;
  int		x,y;
  
  if (scaledviewwidth == screenwidth)
    return;
  
  if(shareware)
    {
      src = W_CacheLumpName ("FLOOR04", PU_CACHE);
    }
  else
    {
      src = W_CacheLumpName ("FLAT513", PU_CACHE);
    }
  dest = screen;
  
  for (y=0 ; y<screenheight-SBARHEIGHT ; y++)
    {
      for (x=0 ; x<screenwidth/64 ; x++)
	{
	  memcpy (dest, src+((y&63)<<6), 64);
	  dest += 64;
	}
      if (screenwidth&63)
	{
	  memcpy (dest, src+((y&63)<<6), screenwidth&63);
	  dest += (screenwidth&63);
	}
    }
  for(x=viewwindowx; x < viewwindowx+viewwidth; x += 16)
    {
      V_DrawPatch(x-X_DISP, viewwindowy-4, W_CacheLumpName("bordt", PU_CACHE));
      V_DrawPatch(x-X_DISP, viewwindowy+viewheight, W_CacheLumpName("bordb", 
							     PU_CACHE));
    }
  for(y=viewwindowy; y < viewwindowy+viewheight; y += 16)
    {
      V_DrawPatch(viewwindowx-4-X_DISP, y, W_CacheLumpName("bordl", PU_CACHE));
      V_DrawPatch(viewwindowx+viewwidth-X_DISP, y, W_CacheLumpName("bordr", 
							    PU_CACHE));
    }
  V_DrawPatch(viewwindowx-4-X_DISP, viewwindowy-4, W_CacheLumpName("bordtl", 
							    PU_CACHE));
  V_DrawPatch(viewwindowx+viewwidth-X_DISP, viewwindowy-4, 
	      W_CacheLumpName("bordtr", PU_CACHE));
  V_DrawPatch(viewwindowx+viewwidth-X_DISP, viewwindowy+viewheight, 
	      W_CacheLumpName("bordbr", PU_CACHE));
  V_DrawPatch(viewwindowx-4-X_DISP, viewwindowy+viewheight, 
	      W_CacheLumpName("bordbl", PU_CACHE));
}

/*
  ==================
  =
  = R_DrawTopBorder
  =
  = Draws the top border around the view for different size windows
  ==================
*/

boolean BorderTopRefresh;

void R_DrawTopBorder (void)
{
  byte	        *src, *dest;
  int		x,y;
  
  if (scaledviewwidth == screenwidth)
    return;
  
  if(shareware)
    {
      src = W_CacheLumpName ("FLOOR04", PU_CACHE);
    }
  else
    {
      src = W_CacheLumpName ("FLAT513", PU_CACHE);
    }
  dest = screen;
  
  for (y=0 ; y<30 ; y++)
    {
      for (x=0 ; x<screenwidth/64 ; x++)
	{
	  memcpy (dest, src+((y&63)<<6), 64);
	  dest += 64;
	}
      if (screenwidth&63)
	{
	  memcpy (dest, src+((y&63)<<6), screenwidth&63);
	  dest += (screenwidth&63);
	}
    }
  if(viewwindowy < 28)
    {
      for(x=viewwindowx; x < viewwindowx+viewwidth; x += 16)
	{
	  V_DrawPatch(x-X_DISP, viewwindowy-4,
		      W_CacheLumpName("bordt", PU_CACHE));
	}
      V_DrawPatch(viewwindowx-4-X_DISP, viewwindowy,
		  W_CacheLumpName("bordl", PU_CACHE));
      V_DrawPatch(viewwindowx+viewwidth-X_DISP, viewwindowy, 
		  W_CacheLumpName("bordr", PU_CACHE));
      V_DrawPatch(viewwindowx-4-X_DISP, viewwindowy+16,
		  W_CacheLumpName("bordl", PU_CACHE));
      V_DrawPatch(viewwindowx+viewwidth-X_DISP, viewwindowy+16, 
		  W_CacheLumpName("bordr", PU_CACHE));
      
      V_DrawPatch(viewwindowx-4-X_DISP, viewwindowy-4,
		  W_CacheLumpName("bordtl", PU_CACHE));
      V_DrawPatch(viewwindowx+viewwidth-X_DISP, viewwindowy-4, 
		  W_CacheLumpName("bordtr", PU_CACHE));
    }
}

void R_DrawSkyColumn (void) {
    int                 count;
    byte                *dest;
    fixed_t             frac, fracstep;

        
    count = dc_yh - dc_yl;
        
    /* Zero length, column does not exceed a pixel. */
    if (count < 0)
	return;
    
#ifdef RANGECHECK
    if ((unsigned)dc_x >= (unsigned)screenwidth || dc_yl < 0 || (unsigned)dc_yh >= (unsigned)screenheight)
	I_Error ("R_DrawSkyColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif
      
    /*
     * Framebuffer destination address.
     * Use ylookup LUT to avoid multiply with ScreenWidth.
     * Use columnofs LUT for subwindows?
     */
    dest = ylookup[dc_yl] + columnofs[dc_x];
    
    /*
     * Determine scaling,
     *  which is the only mapping to be done.
     */
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;
    
    /*
     * Inner loop that does the actual texture mapping,
     *  e.g. a DDA-lile scaling.
     * This is as fast as it gets.
     */
    do {
	/*
	 * Re-map color indices from wall texture column
	 *  using a lighting/special effects LUT.
	 */
	
	*dest = dc_colormap[dc_source[(frac>>FRACBITS)&255]];
	
	dest += screenwidth;
	frac += fracstep;
	
    } while (count--);
}
