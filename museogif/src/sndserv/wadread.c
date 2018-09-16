/*
 *-----------------------------------------------------------------------------
 *
 * $Log: wadread.c,v $
 *
 * Revision 2.0  1999/01/24 00:52:12  Andre Wertmann
 * changed to work with Heretic 
 *
 * Revision 1.7  1998/06/23 13:23:06  rafaj
 * findlump() now always returns defined int as result
 *
 * Revision 1.6  1998/05/17 00:13:58  chris
 * removed debug messages; cleaned up
 *
 * Revision 1.5  1998/04/24 23:37:36  chris
 * read_pwads() and support functions - now sounds from PWAD
 * files are played
 *
 * Revision 1.4  1998/04/24 21:09:04  chris
 * ID's released version
 *
 * Revision 1.3  1997/01/30 19:54:23  b1
 * Final reformatting run. All the remains (ST, W, WI, Z).
 *
 * Revision 1.2  1997/01/21 19:00:10  b1
 * First formatting run:
 *  using Emacs cc-mode.el indentation for C++ now.
 *
 * Revision 1.1  1997/01/19 17:22:51  b1
 * Initial check in DOOM sources as of Jan. 10th, 1997
 *
 *
 * DESCRIPTION:
 *	WAD and Lump I/O, the second.
 *	This time for soundserver only.
 *	Welcome to Department of Redundancy Department. Again :-).
 *
 *-----------------------------------------------------------------------------
 */


#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>

#include "soundsrv.h"
#include "wadread.h"

#ifndef HOMEDIR
#define HOMEDIR "/.heretic"
#endif /* HOMEDIR */


int*		sfxlengths;

typedef struct wadinfo_struct
{
  char	identification[4];
  int   numlumps;
  int	infotableofs;
} wadinfo_t;

typedef struct filelump_struct
{
  int		filepos;
  int		size;
  char	        name[8];
} filelump_t;

typedef struct lumpinfo_struct
{
  int		handle;
  int		filepos;
  int		size;
  char	        name[8];
} lumpinfo_t;

lumpinfo_t*	lumpinfo;
int		numlumps;

void**		lumpcache;


#define stricmp strcasecmp
#define strnicmp strncasecmp


/*
 * Something new.
 * This version of w_wad.c does handle endianess.
 */
#ifdef __BIG_ENDIAN__

#define SHORT(x) ((short)( \
(((unsigned short)(x) & (unsigned short)0x00ffU) << 8) \
| \
(((unsigned short)(x) & (unsigned short)0xff00U) >> 8) ))
     
#define LONG(x) ((int)( \
(((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) \
| \
(((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) \
| \
(((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) \
| \
(((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))
     
#else
#define LONG(x) (x)
#define SHORT(x) (x)
#endif
     

/* Way too many of those... */     
static void derror(char* msg)
{
  fprintf(stderr, "\nwadread error: %s\n", msg);
  exit(-1);
}


void strupr (char *s)
{
  while (*s) {
    *s = toupper(*s); s++;
  }
}

int filelength (int handle)
{
  struct stat	fileinfo;
  
  if (fstat (handle,&fileinfo) == -1)
    fprintf (stderr, "Error fstating\n");
  
  return fileinfo.st_size;
}

char* accesswad( char* wadname)
{
    char* fn;
    char* envhome;
    
    if ((envhome = getenv("HERETICHOME")) != NULL)
	{
	    fn = (char*) malloc(strlen(envhome)+strlen("/")+strlen(wadname)+1);
	    assert(fn);
	    sprintf(fn, "%s/%s", envhome, wadname);
	    if (!access(fn, R_OK))
		return fn;
	    else
		free(fn);
	}
    if ((envhome = getenv("HOME")) != NULL)
	{
	    fn = (char*) malloc(strlen(envhome)+strlen(HOMEDIR"/")
				+strlen(wadname)+1);
	    assert(fn);
	    sprintf(fn, "%s"HOMEDIR"/%s", envhome, wadname);
	    if (!access(fn, R_OK))
		return fn;
	    else
		free(fn);
	}
    if ((envhome = getenv("PATH")))
        {
            char *path = strdup(envhome), *curentry;
            assert(path);                           
            while (strlen(path))
                {
                    if (!(curentry = strrchr(path, ':')))
                        curentry = path;
                    else
                        *curentry++ = 0;
                    fn = (char*)malloc(strlen(curentry)+19+strlen(wadname));   /* Add space for /, ../share/heretic/ */
                    assert(fn);                                 
                    sprintf(fn, "%s/%s", curentry, wadname);
                    if (!access(fn, R_OK))
                        {
                            free(path);
                            return fn;
                        }
                    /* check ../share/heretic */
                    sprintf(fn, "%s/../share/heretic/%s", curentry, wadname);
                    if (!access(fn, R_OK))
                        {
                            free(path);
                            return fn;
                        }
                    free(fn);
                    *curentry = 0;                          
                }
            free(path);
        }        
    if (!access(wadname, R_OK))
	    return wadname;
    
    fn = (char*) malloc(strlen("./")+strlen(wadname)+1);
    assert(fn);
    sprintf(fn,"%s", wadname);
    if (!access(fn ,R_OK))
	return fn;
    else
	free(fn);

    return NULL;
}
			   
void openwad(char* wadname)
{
  
  int		wadfile;
  int		tableoffset;
  int		tablelength;
  int		tablefilelength;
  int		i;
  wadinfo_t	header;
  filelump_t*	filetable;
  
  /* open and read the wadfile header */
  wadfile = open(wadname, O_RDONLY);
  
  if (wadfile < 0)
    derror("Could not open wadfile");
  
  read(wadfile, &header, sizeof header);
  
  if (strncmp(header.identification, "IWAD", 4))
    derror("wadfile has weirdo header");
  
  numlumps = LONG(header.numlumps);
  tableoffset = LONG(header.infotableofs);
  tablelength = numlumps * sizeof(lumpinfo_t);
  tablefilelength = numlumps * sizeof(filelump_t);
  lumpinfo = (lumpinfo_t *) malloc(tablelength);
  assert(lumpinfo);
  filetable = (filelump_t *) ((char*)lumpinfo + tablelength - tablefilelength);
  
  /* get the lumpinfo table */
  lseek(wadfile, tableoffset, SEEK_SET);
  read(wadfile, filetable, tablefilelength);
  
  /* process the table to make the endianness right and shift it down */
  for (i=0 ; i<numlumps ; i++)
    {
      strncpy(lumpinfo[i].name, filetable[i].name, 8);
      lumpinfo[i].handle = wadfile;
      lumpinfo[i].filepos = LONG(filetable[i].filepos);
      lumpinfo[i].size = LONG(filetable[i].size);
      /* fprintf(stderr, "lump [%.8s] exists\n", lumpinfo[i].name); */
    }
  
}

void* loadlump( char* lumpname, int* size )
{
  int		i;
  void*	        lump;
  
  for (i=0 ; i<numlumps ; i++)
    {
      if (!strncasecmp(lumpinfo[i].name, lumpname, 8))
	break;
    }
  
  if (i == numlumps)
    {
      /*
       * fprintf(stderr,
       *   "Could not find lumpname [%s]\n", lumpname);
       */
      lump = 0;
    }
  else
    {
	lump = (void *) malloc(lumpinfo[i].size);
	assert(lump);
	lseek(lumpinfo[i].handle, lumpinfo[i].filepos, SEEK_SET);
	read(lumpinfo[i].handle, lump, lumpinfo[i].size);
	*size = lumpinfo[i].size;
    }
  return lump;  
}

void* getsfx( char* sfxname, int* len )
{
  unsigned char*	sfx;
  unsigned char*	paddedsfx;
  int			i;
  int			size;
  int			paddedsize;
  char		        name[20];
  
  /* sprintf(name, "ds%s", sfxname); */
  sprintf(name, "%s", sfxname);
#ifdef _DEBUGSOUND
  fprintf(stderr, "Name: %s ; Sfxname: %s\n", name, sfxname );
#endif
  
  sfx = (unsigned char *) loadlump(name, &size);
#ifdef _DEBUGSOUND
  fprintf(stderr, "Sfxsize: %d\n", size );
#endif
  
  /* pad the sound effect out to the mixing buffer size */
  paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;
  paddedsfx = (unsigned char *) realloc(sfx, paddedsize+8);
  for (i=size ; i<paddedsize+8 ; i++)
    paddedsfx[i] = 128;
  
  *len = paddedsize;
  return (void *) (paddedsfx + 8);  
}

static int findlump(char *lumpname)
{
  int i;
  
  for (i=0; i<numlumps; i++)
    if (!strnicmp(lumpname,lumpinfo[i].name,6))
      return(i);
  return(-1);
}

static void do_pwad(char* wadname)
{
  int		wadfile;
  int		tableoffset;
  int		tablelength;
  int		tablefilelength;
  int		i,j;
  int           _numlumps;
  lumpinfo_t    *_lumpinfo;
  wadinfo_t	header;
  filelump_t*	filetable;
  
  /* open and read the wadfile header */
  wadfile = open(wadname, O_RDONLY);
  
  if (wadfile < 0)
    derror("Could not open wadfile");
  
  read(wadfile, &header, sizeof header);
  
  if (strncmp(header.identification, "PWAD", 4) &&
      strncmp(header.identification, "IWAD", 4)) /* e.g. tnt.wad is a IWAD */
    derror("wadfile has weirdo header");
  
  _numlumps = LONG(header.numlumps);
  tableoffset = LONG(header.infotableofs);
  tablelength = numlumps * sizeof(lumpinfo_t);
  tablefilelength = numlumps * sizeof(filelump_t);
  _lumpinfo = (lumpinfo_t *) malloc(tablelength);
  assert(_lumpinfo);
  filetable = (filelump_t *) ((char*)_lumpinfo + tablelength - tablefilelength);
  
  /* get the lumpinfo table */
  lseek(wadfile, tableoffset, SEEK_SET);
  read(wadfile, filetable, tablefilelength);
  
  /* process the table to make the endianness right and shift it down */
  for (i=0 ; i<_numlumps ; i++)
    {
      if (strnicmp(filetable[i].name,"DS",2)) continue;
      j=findlump(filetable[i].name);
      if (j == -1) {
	char lname[7];
	strncpy(lname,filetable[i].name,6);
	fprintf(stderr,"sndserver: unidentified lump '%s' -- ignored\n",lname);
	continue;
      }
      else {
	char lname[7],lname2[7];
	strncpy(lname,filetable[i].name,6);
	strncpy(lname2,lumpinfo[j].name,6);
	lname[6]=lname2[6]=0;
	/* fprintf(stderr,"sndserver: replacing '%s' '%s'\n",lname,lname2); */
      }
      lumpinfo[j].handle = wadfile;
      lumpinfo[j].filepos = LONG(filetable[i].filepos);
      lumpinfo[j].size = LONG(filetable[i].size);
      /* fprintf(stderr, "lump [%.8s] exists\n", lumpinfo[i].name); */
    }
  free(_lumpinfo);
}

/*
 * inspired/stolen by/from read_extra_wads() from musserver-1.22
 * this shouldn't be needed, doom should tell sndserver  which
 * wads to use.
 */

void read_pwads(void)
{
    pid_t ppid;
    char filename[100];
    FILE *fp;
    
    ppid=getppid();    /* heretic's pid */
    sprintf(filename,"/proc/%d/cmdline",(int)ppid);
    
    /* fprintf(stderr,"sndserver: in read_pwads\n"); */
    
    if ((fp = fopen(filename, "r"))) {
      char *s;
      static char buff[256];
      int len, active = 0;
      len = fread(buff, 1, 256, fp);
      fclose(fp);
      for (s = buff; s < buff+len;) {
	if (*s == '-')
	  active = 0;
	if (active)
                do_pwad(s);
	else if (!strcmp(s, "-file"))
	  active = 1;
            s += strlen(s) + 1;
      }
    }
}

