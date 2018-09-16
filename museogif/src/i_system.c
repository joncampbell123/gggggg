
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "doomdef.h"
#include "i_sound.h"


char* homedir;
char* basedefault;


long	mb_used = 8;
int     DisplayTicker = 0;


void I_Tactile(int __attribute__((unused)) on,int __attribute__((unused)) off,int __attribute__((unused)) total)
{
}

ticcmd_t emptycmd;

ticcmd_t* I_BaseTiccmd(void)
{
  return &emptycmd;
}


int  I_GetHeapSize (void)
{
  return mb_used*1024*1024;
}

byte *I_ZoneBase (size_t *size)
{
    byte *dummy;
    
    *size = mb_used*1024*1024;
    dummy = (byte *) malloc (*size);
    assert(dummy);
    return dummy;
}


/*
 * I_GetTime
 * returns time in 1/70th second tics
 */
int  I_GetTime (void)
{
  struct timeval	tp;
  struct timezone	tzp;
  int			newtics;
  static int		basetime=0;
  
  gettimeofday(&tp, &tzp);
  if (!basetime)
    basetime = tp.tv_sec;
  newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
  /* printf("Newtics: %d\n",newtics); */
  return newtics;
}


/* sets and/or gets your private Heretic-homedirectory */

void I_GetHomeDirectory(void)
{
  char* dummy;
/*  int rcode;*/

  if ((dummy=getenv("HOME")) != NULL)
      {
	  homedir= (char*) malloc(strlen(dummy)+strlen("/.heretic/")+1);
	  assert(homedir);
	  sprintf(homedir, "%s/.heretic/", dummy);
      }
  else
      {
	  homedir = strdup("./");
      }
  
  /* mode: drwxr-xr-x (16877dec) */
  /*rcode=*/mkdir(homedir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 
  
  basedefault= (char*) malloc(strlen(homedir)+strlen("heretic.cfg")+1);
  assert(basedefault);
  sprintf(basedefault, "%sheretic.cfg", homedir);   
}


/*
 * I_Init
 */
void I_Init (void)
{
  if (! M_CheckParm("-nosound"))
    {
#ifdef __DOSOUND__
      I_InitSound();
#endif

#ifdef _DEBUGSOUND
      fprintf(stderr, "FIXME, Calling I_InitSound...\n");
#endif
    }
  
  else
    fprintf(stderr,"I_Init: sound disabled (-nosound)\n");
  if (! M_CheckParm("-nomusic"))
    {
#ifdef __DOMUSIC__
      I_InitMusic();
#endif

#ifdef _DEBUGSOUND
      fprintf(stderr, "FIXME, Calling I_InitMusic...\n");
#endif
    }
  else
    fprintf(stderr,"I_Init: backgound music disabled (-nomusic)\n");
}


/*
 * I_Quit
 */
void I_Quit (void)
{
  D_QuitNetGame ();
#ifdef __DOSOUND__
  I_ShutdownSound();
#endif

#ifdef _DEBUGSOUND
  fprintf(stderr, "(working?): Calling I_ShutdownSound...\n");
#endif

#ifdef __DOMUSIC__
  I_ShutdownMusic();
#endif

#ifdef _DEBUGSOUND
  fprintf(stderr, "FIXME, Calling I_ShutdownMusic...\n");
#endif
  
  M_SaveDefaults ();
  I_ShutdownGraphics();
  
  free(homedir); free(basedefault);
  exit(0);
}

void I_WaitVBL(int count)
{
  usleep (count * (1000000/70) );
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

#ifndef UNIX
byte* I_AllocLow(int length)
{
  byte* mem;
  
  mem = (byte *)malloc (length);
  assert( mem );
  if (mem) memset (mem,0,length);
  return mem;
}
#endif

/*
 * I_Error
 */
extern boolean demorecording;

#define _ERROR_HDR "Error: "

void I_Error (char *error, ...)
{
  va_list     argptr;
  char        err_buff[1024];
  
  /* put message first into buffer */
  va_start (argptr,error);
  strcpy(err_buff,_ERROR_HDR);
  vsprintf(err_buff+strlen(_ERROR_HDR),error,argptr);
  strcat(err_buff,"\n");
  va_end (argptr);
  
  /* Shutdown. Here might be other errors. */
  if (demorecording)
    G_CheckDemoStatus();
  
  D_QuitNetGame ();

#ifdef __DOSOUND__
  I_ShutdownSound();
#endif

#ifdef _DEBUGSOUND
  fprintf(stderr, "FIXME, Calling I_ShutdownSound...\n");
#endif

#ifdef __DOMUSIC__
  I_ShutdownMusic();
#endif

#ifdef _DEBUGSOUND
  fprintf(stderr, "FIXME, Calling I_ShutdownMusic...\n");
#endif
  
  I_ShutdownGraphics();

  free(homedir); free(basedefault);
  
  /* now print the error to stderr */
  fprintf(stderr,"%s",err_buff);
  fprintf(stderr, "\nYou can send this bug message to: heretic@awerthmann.de\n\n");
  fflush( stderr );
  
  exit(-1);
}


