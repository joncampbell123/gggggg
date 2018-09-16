/*
 *-----------------------------------------------------------------------------
 *
 * $Log: soundsrv.c,v $
 *
 * Revision 2.0  1999/01/24 00:52:12  Andre Wertmann
 * changed to work with Heretic 
 *
 * Revision 1.6  1998/06/23 11:42:38  rafaj
 * grabdata() now recognises plutonia.wad and tnt.wad
 *
 * Revision 1.5  1998/04/24 23:36:29  chris
 * call read_pwads() after limp list was created
 *
 * Revision 1.4  1998/04/24 21:01:52  chris
 * this is ID's released version
 *
 * Revision 1.3  1997/01/29 22:40:44  b1
 * Reformatting, S (sound) module files.
 *
 * Revision 1.2  1997/01/21 19:00:07  b1
 * First formatting run:
 *  using Emacs cc-mode.el indentation for C++ now.
 *
 * Revision 1.1  1997/01/19 17:22:50  b1
 * Initial check in DOOM sources as of Jan. 10th, 1997
 *
 *
 * DESCRIPTION:
 *	UNIX soundserver, run as a separate process,
 *	 started by DOOM program.
 *	Originally conceived fopr SGI Irix,
 *	 mostly used with Linux voxware.
 *
 *-----------------------------------------------------------------------------
 */


#include <math.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "sounds.h"
#include "soundsrv.h"
#include "wadread.h"

#define MIX_CHANNELS	16

/*
 * Department of Redundancy Department.
 */
typedef struct wadinfo_struct
{
  /* should be IWAD */
  char	identification[4];
  int		numlumps;
  int		infotableofs;
  
} wadinfo_t;


typedef struct filelump_struct
{
  int		filepos;
  int		size;
  char	name[8];
  
} filelump_t;


/* an internal time keeper */
static int	mytime = 0;

/* number of sound effects */
int 		numsounds;
 
/* longest sound effect */
int 		longsound;

/* lengths of all sound effects */
int 		lengths[NUMSFX];

/* mixing buffer */
signed short	mixbuffer[MIXBUFFERSIZE];

/* file descriptor of sfx device */
int		sfxdevice;

/* file descriptor of music device */
int 		musdevice;

/* the channel data pointers */
unsigned char*	channels[MIX_CHANNELS] = {NULL};

/* the channel step amount */
unsigned int	channelstep[MIX_CHANNELS] = {0};

/* 0.16 bit remainder of last step */
unsigned int	channelstepremainder[MIX_CHANNELS] = {0};

/* the channel data end pointers */
unsigned char*	channelsend[MIX_CHANNELS] = {NULL};

/* time that the channel started playing */
int		channelstart[MIX_CHANNELS] = {0};

/* the channel handles */
int 		channelhandles[MIX_CHANNELS] = {0};

/* the channel left volume lookup */
int*		channelleftvol_lookup[MIX_CHANNELS] = {0};

/* the channel right volume lookup */
int*		channelrightvol_lookup[MIX_CHANNELS] = {0};

/* sfx id of the playing sound effect */
int		channelids[MIX_CHANNELS] = {0};

int		snd_verbose=1;

int		steptable[256];

int		vol_lookup[128*256];

static void derror(char* msg)
{
  fprintf(stderr, "error: %s\n", msg);
  exit(-1);
}

int mix(void)
{

  register int		dl;
  register int		dr;
  register unsigned int	sample;
  
  signed short*		leftout;
  signed short*		rightout;
  signed short*		leftend;
  
  int			step,ch;
  
  leftout = mixbuffer;
  rightout = mixbuffer+1;
  step = 2;
  
  leftend = mixbuffer + SAMPLECOUNT*step;
  
  /* mix into the mixing buffer */
  while (leftout != leftend)
    {
      
      dl = 0;
      dr = 0;
      
      for (ch=0;ch < MIX_CHANNELS;ch++) {
        if (channels[ch]) {
	  sample = *channels[ch];
	  dl += channelleftvol_lookup[ch][sample];
	  dr += channelrightvol_lookup[ch][sample];
	  channelstepremainder[ch] += channelstep[ch];
	  channels[ch] += channelstepremainder[ch] >> 16;
	  channelstepremainder[ch] &= 65536-1;
	  
	  if (channels[ch] >= channelsend[ch])
	    channels[ch] = 0;
	}
      }
     
      /*
       * Has been char instead of short.
       * if (dl > 127) *leftout = 127;
       * else if (dl < -128) *leftout = -128;
       * else *leftout = dl;
       
       * if (dr > 127) *rightout = 127;
       * else if (dr < -128) *rightout = -128;
       * else *rightout = dr;
       */

      if (dl > 0x7fff)
	*leftout = 0x7fff;
      else if (dl < -0x8000)
	*leftout = -0x8000;
      else
	*leftout = dl;
      
      if (dr > 0x7fff)
	*rightout = 0x7fff;
      else if (dr < -0x8000)
	*rightout = -0x8000;
      else
	*rightout = dr;
      
      leftout += step;
      rightout += step;
      
    }
  return 1;
}



void grabdata( int c, char** v )
{
  int		i;
  char*	        name;
  char          hereticsharewad[] = "heretic_share.wad";
  char          hereticfullwad[] = "heretic.wad";

  
  /*
   *	home = getenv("HOME");
   *	if (!home)
   *	  derror("Please set $HOME to your home directory");
   *	sprintf(basedefault, "%s/.doomrc", home);
   */
  
  for (i=1 ; i<c ; i++)
    {
      if (!strcmp(v[i], "-quiet"))
	{
	  snd_verbose = 0;
	}
    }

  numsounds = NUMSFX;
  longsound = 0;

  name = accesswad(hereticfullwad);
  if (name == NULL)
      name = accesswad(hereticsharewad);
  if (name == NULL)
      {
	  fprintf(stderr, "Could not find wadfile anywhere\n");
	  exit(-1);
      }
  
  
  openwad(name);
  if (snd_verbose)
      fprintf(stderr, "sndserver: loading from [%s]\n", name);
  
  read_pwads();   /* do PWAD processing */
  
  for (i=1 ; i<NUMSFX ; i++)
    {
      if (!S_sfx[i].link)
	{
	  S_sfx[i].data = getsfx(S_sfx[i].name, &lengths[i]);
	  if (longsound < lengths[i]) longsound = lengths[i];
	} else {
	  S_sfx[i].data = S_sfx[i].link->data;
	  lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
	}
    }
}

static struct timeval		last={0,0};

static struct timezone		whocares;

void updatesounds(void)
{
  mix();
  I_SubmitOutputBuffer(mixbuffer, SAMPLECOUNT); 
}

int addsfx( int sfxid, int volume, int step, int seperation )
{
  static unsigned short	handlenums = 0;
  
  int		i;
  int		rc = -1;
  
  int		oldest = mytime;
  int		oldestnum = 0;
  int		slot;
  int		rightvol;
  int		leftvol;

  /*
   * play these sound effects
   *  only one at a time
   */

  /*
    if ( sfxid == sfx_sawup
    || sfxid == sfx_sawidl
    || sfxid == sfx_sawful
    || sfxid == sfx_sawhit
    || sfxid == sfx_stnmov
    || sfxid == sfx_pistol )
  */

#if 0 /* removed; do we really need this ? */
  if ( sfxid == sfx_gntful
       || sfxid == sfx_gnthit
       || sfxid == sfx_stnmov )
    
    {
      for (i=0 ; i<8 ; i++)
	{
	  if (channels[i] && channelids[i] == sfxid)
	    {
	      channels[i] = 0;
	      break;
	    }
	}
    }
#endif
  
  for (i=0 ; i<8 && channels[i] ; i++)
    {
      if (channelstart[i] < oldest)
	{
	  oldestnum = i;
	  oldest = channelstart[i];
	}
    }
  
  if (i == 8)
    slot = oldestnum;
  else
    slot = i;
  
  channels[slot] = (unsigned char *) S_sfx[sfxid].data;
  channelsend[slot] = channels[slot] + lengths[sfxid];
  
  if (!handlenums)
    handlenums = 100;
  
  channelhandles[slot] = rc = handlenums++;
  channelstep[slot] = step;
  channelstepremainder[slot] = 0;
  channelstart[slot] = mytime;
  
  /* (range: 1 - 256) */
  seperation += 1;
  
  /* printf("seperation before leftvol: %d\n", seperation); */
  
  /* (x^2 seperation) */
  leftvol =
    volume - (volume*seperation*seperation)/(256*256);

  if (leftvol<0) 
    {
#ifdef _DEBUGSOUND
      printf("warning: leftvol<0 !: %d\n", leftvol);
#endif
      leftvol=0; 
    }
  if (leftvol>127)
    {
#ifdef _DEBUGSOUND
      printf("warning: leftvol>127 !: %d\n", leftvol);
#endif
      leftvol=127;
    }
  
  /* printf("leftvol: %d\n", leftvol); */
  
  seperation = seperation - 257;
  
  /* printf("seperation before rightvol: %d\n", seperation); */

  /* (x^2 seperation) */
  rightvol =
    volume - (volume*seperation*seperation)/(256*256);

  if (rightvol<0)
    {
#ifdef _DEBUGSOUND
      printf("warning: rightvol<0 !: %d\n", rightvol);
#endif
      rightvol=0;
    }

  if (rightvol>127)
    {
#ifdef _DEBUGSOUND
      printf("warning: rightvol>127 !: %d\n", rightvol);
#endif
      rightvol=127;
    }

  /* printf("rightvol: %d\n", rightvol); */
  
  /* sanity check */
  if (rightvol < 0 || rightvol > 127)
    derror("rightvol out of bounds");
  
  if (leftvol < 0 || leftvol > 127)
    derror("leftvol out of bounds");
  
  /*
   * get the proper lookup table piece
   *  for this volume level
   */
  channelleftvol_lookup[slot] = &vol_lookup[leftvol*256];
  channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];
  
  channelids[slot] = sfxid;
  
  return rc;  
}


void outputushort(int num)
{
  static unsigned char	buff[5] = { 0, 0, 0, 0, '\n' };
  static char*		badbuff = "xxxx\n";
  
  /* outputs a 16-bit # in hex or "xxxx" if -1. */
  if (num < 0)
    {
      write(1, badbuff, 5);
    }
  else
    {
      buff[0] = num>>12;
      buff[0] += buff[0] > 9 ? 'a'-10 : '0';
      buff[1] = (num>>8) & 0xf;
      buff[1] += buff[1] > 9 ? 'a'-10 : '0';
      buff[2] = (num>>4) & 0xf;
      buff[2] += buff[2] > 9 ? 'a'-10 : '0';
      buff[3] = num & 0xf;
      buff[3] += buff[3] > 9 ? 'a'-10 : '0';
      write(1, buff, 5);
    }
}

void initdata(void)
{
  int		i;
  int		j;
  
  int*	        steptablemid = steptable + 128;
  
  for (i=0 ;
       i<sizeof(channels)/sizeof(unsigned char *) ;
       i++)
    {
      channels[i] = 0;
    }
  
  gettimeofday(&last, &whocares);
  
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = pow(2.0, (i/64.0))*65536.0;
  
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/127;
  
}


void quit(void)
{
  /* I_ShutdownMusic(); */
  I_ShutdownSound();
  exit(0);
}

fd_set		fdset;
fd_set		scratchset;


int main( int c, char**	v )
{
  int		done = 0;
  int		rc;
  int		nrc;
  int		sndnum;
//  int		handle = 0;
  
  unsigned char	commandbuf[10];
  struct timeval	zerowait = { 0, 0 };
  
  
  int 	        step;
  int 	        vol;
  int		sep;
  
  int		i;
  int		waitingtofinish=0;
  
  /* get sound data */
  grabdata(c, v);
  
  /* init any data */
  initdata();
  
  I_InitSound(11025, 16);
  
  /* I_InitMusic(); */
  
  if (snd_verbose)
    fprintf(stderr, "sndserver: ready\n");
  
  /* parse commands and play sounds until done */
  FD_ZERO(&fdset);
  FD_SET(0, &fdset);

  while (!done)
    {
      mytime++;
      
      if (!waitingtofinish)
	{
	  do {
	    scratchset = fdset;
	    rc = select(FD_SETSIZE, &scratchset, 0, 0, &zerowait);
	    
	    if (rc > 0)
	      {
		/*
		 *	fprintf(stderr, "select is true\n");
		 *   got a command
		 */
		nrc = read(0, commandbuf, 1);
		
		if (!nrc)
		  {
		    done = 1;
		    rc = 0;
		  }
		else
		  {
		    if (snd_verbose)
		      fprintf(stderr, "cmd: %c", commandbuf[0]);
		    
		    switch (commandbuf[0])
		      {
		      case 'p':
			/* play a new sound effect */
			read(0, commandbuf, 9);
			
			if (snd_verbose)
			  {
			    commandbuf[9]=0;
			    fprintf(stderr, "%s\n", commandbuf);
			  }
			
			commandbuf[0] -=
			  commandbuf[0]>='a' ? 'a'-10 : '0';
			commandbuf[1] -=
			  commandbuf[1]>='a' ? 'a'-10 : '0';
			commandbuf[2] -=
			  commandbuf[2]>='a' ? 'a'-10 : '0';
			commandbuf[3] -=
			  commandbuf[3]>='a' ? 'a'-10 : '0';
			commandbuf[4] -=
			  commandbuf[4]>='a' ? 'a'-10 : '0';
			commandbuf[5] -=
			  commandbuf[5]>='a' ? 'a'-10 : '0';
			commandbuf[6] -=
			  commandbuf[6]>='a' ? 'a'-10 : '0';
			commandbuf[7] -=
			  commandbuf[7]>='a' ? 'a'-10 : '0';
			
			/*	p<snd#><step><vol><sep> */
			sndnum = (commandbuf[0]<<4) + commandbuf[1];
			step = (commandbuf[2]<<4) + commandbuf[3];
			step = steptable[step];
			vol = (commandbuf[4]<<4) + commandbuf[5];
			sep = (commandbuf[6]<<4) + commandbuf[7];
			
			/*handle = */addsfx(sndnum, vol, step, sep);
			/*
			 * returns the handle
			 *	outputushort(handle);
			 */
			break;
			
		      case 'q':
			read(0, commandbuf, 1);
			waitingtofinish = 1; rc = 0;
			break;
			
		      case 's':
			{
			  int fd;
			  read(0, commandbuf, 3);
			  commandbuf[2] = 0;
			  fd = open((char*)commandbuf, O_CREAT|O_WRONLY, 0644);
			  commandbuf[0] -= commandbuf[0]>='a' ? 'a'-10 : '0';
			  commandbuf[1] -= commandbuf[1]>='a' ? 'a'-10 : '0';
			  sndnum = (commandbuf[0]<<4) + commandbuf[1];
			  write(fd, S_sfx[sndnum].data, lengths[sndnum]);
			  close(fd);
			}
			break;
			
		      default:
#ifdef _DEBUGSOUND
			fprintf(stderr, "Did not recognize command\n");
#endif
			break;
		      }
		  }
	      }
	    else if (rc < 0)
	      {
		quit();
	      }
	  } while (rc > 0);
	}
      
      updatesounds();
      
      if (waitingtofinish)
	{
	  for(i=0 ; i<8 && !channels[i] ; i++);
	  
	  if (i==8)
	    done=1;
	}
      
    }
  
  quit();
  return 0;
}

