/* DoomData.h */

/*
 * all external data is defined here
 * most of the data is loaded into different structures at run time.
 */

#ifndef __DOOMDATA__
#define __DOOMDATA__

#include "doomtype.h"
#include "doomdef.h"

/*
  ===============================================================================
  
  map level types

  ===============================================================================
*/

/* lump order in a map wad */
enum 
{
  ML_LABEL, 
  ML_THINGS, 
  ML_LINEDEFS, 
  ML_SIDEDEFS, 
  ML_VERTEXES, 
  ML_SEGS,
  ML_SSECTORS, 
  ML_NODES, 
  ML_SECTORS, 
  ML_REJECT, 
  ML_BLOCKMAP
};

typedef struct
{
  short		x,y;
} mapvertex_t;

typedef struct
{
  short		textureoffset;
  short		rowoffset;

  char          toptexture[8]  __PACKED__ ,
		bottomtexture[8]  __PACKED__ ,
		midtexture[8]  __PACKED__ ;
  
  short         sector  __PACKED__ ;
    /* on viewer's side */
}   __PACKED__ mapsidedef_t;

typedef struct
{
  short		v1, v2;
  short		flags;
  short		special, tag;
  short		sidenum[2];			/* sidenum[1] will be -1 if one sided */
}  __PACKED__ maplinedef_t;

#define	ML_BLOCKING	        1
#define	ML_BLOCKMONSTERS	2
#define	ML_TWOSIDED		4		/* backside will not be present at all */
/* if not two sided */

/*
 * if a texture is pegged, the texture will have the end exposed to air held
 * constant at the top or bottom of the texture (stairs or pulled down things)
 * and will move with a height change of one of the neighbor sectors
 * Unpegged textures allways have the first row of the texture at the top
 * pixel of the line for both top and bottom textures (windows)
 */
#define	ML_DONTPEGTOP		8
#define	ML_DONTPEGBOTTOM	16

#define ML_SECRET		32	/* don't map as two sided: IT'S A SECRET! */
#define ML_SOUNDBLOCK		64	/* don't let sound cross two of these */
#define	ML_DONTDRAW		128	/* don't draw on the automap */
#define	ML_MAPPED		256	/* set if allready drawn in automap */

typedef	struct
{
  short		floorheight, ceilingheight;

  char          floorpic[8], ceilingpic[8]  __PACKED__ ;
  short         lightlevel  __PACKED__ ;
  short         special  __PACKED__ , tag  __PACKED__ ;
}  __PACKED__  mapsector_t;

typedef struct
{
  short		numsegs;
  short		firstseg;		/* segs are stored sequentially */
} mapsubsector_t;

typedef struct
{
  short		v1, v2;
  short		angle;		
  short		linedef, side;
  short		offset;
} mapseg_t;

enum
{
  BOXTOP,
  BOXBOTTOM,
  BOXLEFT,
  BOXRIGHT 
};	/* bbox coordinates */

#define	NF_SUBSECTOR	0x8000
typedef struct
{
  short		x,y,dx,dy;		      /* partition line */
  short		bbox[2][4];		      /* bounding box for each child */
  unsigned short    children[2];	      /* if NF_SUBSECTOR its a subsector */
}  __PACKED__  mapnode_t;

typedef struct
{
  short		x,y;
  short		angle;
  short		type;
  short		options;
}  __PACKED__  mapthing_t;

#define	MTF_EASY		1
#define	MTF_NORMAL		2
#define	MTF_HARD		4
#define	MTF_AMBUSH		8

/*
  ===============================================================================
  
  texture definition
  
  ===============================================================================
*/

typedef struct
{
  short	originx;
  short	originy;
  short	patch;
  short	stepdir;
  short	colormap;
}  __PACKED__  mappatch_t;

typedef struct
{
  char		name[8];
  boolean       masked  __PACKED__ ;
  short         width  __PACKED__ ;
  short         height  __PACKED__ ;
#if 1
  int32_t	columndirectory  __PACKED__ ; /* OBSOLETE */
#else
  void          **columndirectory  __PACKED__ ;     /* OBSOLETE */
#endif 
  short         patchcount  __PACKED__ ;
  mappatch_t    patches[1]  __PACKED__ ;
}  __PACKED__  maptexture_t;


/*
  ===============================================================================
  
  graphics
  
  ===============================================================================
*/

/* posts are runs of non masked source pixels */
typedef struct
{
  byte        topdelta;      /* -1 is the last post in a column */
  byte        length  __PACKED__ ;
  /* length data bytes follows */
}  __PACKED__  post_t;

/* column_t is a list of 0 or more post_t, (byte)-1 terminated */
typedef post_t	column_t;

/*
 * a patch holds one or more columns
 * patches are used for sprites and all masked pictures
 */
typedef struct
{
  short		width;				/* bounding box size */
  short		height;
  short		leftoffset;			/* pixels to the left of origin */
  short		topoffset;			/* pixels below the origin */
  int		columnofs[8];		        /* only [width] used */
  /* the [0] is &columnofs[width] */
}   __PACKED__  patch_t;

/* a pic is an unmasked block of pixels */
typedef struct
{
  byte           width,height  __PACKED__ ;
  byte           data  __PACKED__ ;
}  __PACKED__  pic_t;

/*
  ===============================================================================
  
  status
  
  ===============================================================================
*/

#endif   /* __DOOMDATA__ */
