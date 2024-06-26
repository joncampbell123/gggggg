
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>

#ifndef UNIX
#include <sys/filio.h>
#endif /* UNIX */

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "i_sound.h"
#include "doomdef.h"

/* UNIX hack, to be removed. */
#ifdef SNDSERV
/* Separate sound server process. */
FILE*	sndserver=0;
char*	sndserver_filename = "./sndserver";
char*	sndserver_options = "-quiet";
#endif /* SNDSERV */

/*
 * The number of internal mixing channels,
 *  the samples calculated for each mixing step,
 *  the size of the 16bit, 2 hardware channel (stereo)
 *  mixing buffer, and the samplerate of the raw data.
 */

/* Needed for calling the actual sound output. */
#define SAMPLECOUNT		512
#define NUM_CHANNELS		8
/* It is 2 for 16bit, and 2 for two channels. */
#define BUFMUL                  4
#define MIXBUFFERSIZE		(SAMPLECOUNT*BUFMUL)

#define SAMPLERATE		11025	/* Hz */
#define SAMPLESIZE		2   	/* 16bit */

/* The actual lengths of all sound effects. */
int 		lengths[NUMSFX];

/* The actual output device. */
int	        audio_fd;

/*
 * The global mixing buffer.
 * Basically, samples from all active internal channels
 *  are modifed and added, and stored in the buffer
 *  that is submitted to the audio device.
 */
signed short	mixbuffer[MIXBUFFERSIZE];


/* The channel step amount... */
unsigned int	channelstep[NUM_CHANNELS];
/* ... and a 0.16 bit remainder of last step. */
unsigned int	channelstepremainder[NUM_CHANNELS];


/* The channel data pointers, start and end. */
unsigned char*	channels[NUM_CHANNELS];
unsigned char*	channelsend[NUM_CHANNELS];


/*
 * Time/gametic that the channel started playing,
 *  used to determine oldest, which automatically
 *  has lowest priority.
 * In case number of active sounds exceeds
 *  available channels.
 */
int		channelstart[NUM_CHANNELS];

/*
 * The sound in channel handles,
 *  determined on registration,
 *  might be used to unregister/stop/modify,
 *  currently unused.
 */
int 		channelhandles[NUM_CHANNELS];

/*
 * SFX id of the playing sound effect.
 * Used to catch duplicates (like chainsaw).
 */
int		channelids[NUM_CHANNELS];

/* Pitch to stepping lookup, unused. */
int		steptable[256];

/* Volume lookups. */
int		vol_lookup[128*256];

/* Hardware left and right channel volume lookup. */
int*		channelleftvol_lookup[NUM_CHANNELS];
int*		channelrightvol_lookup[NUM_CHANNELS];



void I_SetChannels()
{
  /*
   * Init internal lookups (raw data, mixing buffer, channels).
   * This function sets up internal lookups used during
   *  the mixing process.
   */
  int		i;
  int		j;

  /*
   * Generates volume lookup tables
   *  which also turn the unsigned samples
   *  into signed samples.
   */
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/127;
}


void I_SetSfxVolume(int volume)
{
  /*
   * Identical to DOS.
   * Basically, this should propagate
   *  the menu/config file setting
   *  to the state variable used in
   *  the mixing.
   */
  snd_SfxVolume = volume;
}

/*
 * Retrieve the raw data lump index
 *  for a given SFX name.
 */
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
  char namebuf[9];
  sprintf(namebuf, "%s", sfx->name);
  return W_GetNumForName(namebuf);
}

/*
 * Starting a sound means adding it
 *  to the current list of active sounds
 *  in the internal channels.
 * As the SFX info struct contains
 *  e.g. a pointer to the raw data,
 *  it is ignored.
 * As our sound handling does not handle
 *  priority, it is ignored.
 * Pitching (that is, increased speed of playback)
 *  is set, but currently not used by mixing.
 */
int I_StartSound( int id, int vol, int sep, int pitch, int __attribute__((unused)) priority )
{
#ifdef SNDSERV
  if (sndserver)
    {
      fprintf(sndserver, "p%2.2x%2.2x%2.2x%2.2x\n", id, pitch, vol*8, sep);
      fflush(sndserver);
    }
  /* warning: control reaches end of non-void function. */
#endif
  return id;
}

void I_StopSound (int __attribute__((unused)) handle)
{
  /*
   * You need the handle returned by StartSound.
   * Would be looping all channels,
   *  tracking down the handle,
   *  an setting the channel to zero.
   */
}

int I_SoundIsPlaying(int handle)
{
  /* Ouch. */
  return gametic < handle;
}



/*
 * This function loops all active (internal) sound
 *  channels, retrieves a given number of samples
 *  from the raw sound data, modifies it according
 *  to the current (internal) channel parameters,
 *  mixes the per channel samples into the global
 *  mixbuffer, clamping it to the allowed range,
 *  and sets up everything for transferring the
 *  contents of the mixbuffer to the (two)
 *  hardware channels (left and right, that is).
 *
 * This function currently supports only 16bit.
 */
void I_UpdateSound( void )
{
  /*
   * Mix current sound data.
   * Data, from raw sound, for right and left.
   */
  register unsigned int	sample;
  register int		dl;
  register int		dr;
  
  /* Pointers in global mixbuffer, left, right, end. */
  signed short*		leftout;
  signed short*		rightout;
  signed short*		leftend;
  /* Step in mixbuffer, left and right, thus two. */
  int			step;
  
  /* Mixing channel index. */
  int			chan;
  
  /*
   * Left and right channel
   *  are in global mixbuffer, alternating.
   */
  leftout = mixbuffer;
  rightout = mixbuffer+1;
  step = 2;
  
    /*
     * Determine end, for left channel only
     *  (right channel is implicit).
     */
    leftend = mixbuffer + SAMPLECOUNT*step;
    
    /*
     * Mix sounds into the mixing buffer.
     * Loop over step*SAMPLECOUNT,
     *  that is 512 values for two channels.
     */
    while (leftout != leftend)
      {
	/* Reset left/right value. */
	dl = 0;
	dr = 0;
	
	/*
	 * Love thy L2 chache - made this a loop.
	 * Now more channels could be set at compile time
	 *  as well. Thus loop those  channels.
	 */
	for ( chan = 0; chan < NUM_CHANNELS; chan++ )
	  {
	    /* Check channel, if active. */
	    if (channels[ chan ])
	      {
		/* Get the raw data from the channel. */
		sample = *channels[ chan ];
		/*
		 * Add left and right part
		 *  for this channel (sound)
		 *  to the current data.
		 * Adjust volume accordingly.
		 */
		dl += channelleftvol_lookup[ chan ][sample];
		dr += channelrightvol_lookup[ chan ][sample];
		/* Increment index ??? */
		channelstepremainder[ chan ] += channelstep[ chan ];
		/* MSB is next sample??? */
		channels[ chan ] += channelstepremainder[ chan ] >> 16;
		/* Limit to LSB??? */
		channelstepremainder[ chan ] &= 65536-1;
		
		/* Check whether we are done. */
		if (channels[ chan ] >= channelsend[ chan ])
		  channels[ chan ] = 0;
	      }
	  }
	
	/*
	 * Clamp to range. Left hardware channel.
	 * Has been char instead of short.
	 * if (dl > 127) *leftout = 127;
	 * else if (dl < -128) *leftout = -128;
	 * else *leftout = dl;
	 */
	
	if (dl > 0x7fff)
	  *leftout = 0x7fff;
	else if (dl < -0x8000)
	  *leftout = -0x8000;
	else
	  *leftout = dl;
	
	/* Same for right hardware channel. */
	if (dr > 0x7fff)
	  *rightout = 0x7fff;
	else if (dr < -0x8000)
	  *rightout = -0x8000;
	else
	  *rightout = dr;
	
	/* Increment current pointers in mixbuffer. */
	leftout += step;
	rightout += step;
      }
}


/*
 * This would be used to write out the mixbuffer
 *  during each game loop update.
 * Updates sound buffer and audio device at runtime.
 * It is called during Timer interrupt with SNDINTR.
 * Mixing now done synchronous, and
 *  only output be done asynchronous?
 */
void I_SubmitSound(void)
{
  /* Write it to DSP device. */
  write(audio_fd, mixbuffer, SAMPLECOUNT*BUFMUL);
}


void I_UpdateSoundParams( int __attribute__((unused)) handle, int __attribute__((unused)) vol, int __attribute__((unused)) sep, int __attribute__((unused)) pitch )
{
  /*
   * I fail too see that this is used.
   * Would be using the handle to identify
   *  on which channel the sound might be active,
   *  and resetting the channel parameters.
   */
}


void I_ShutdownSound(void)
{
#ifdef SNDSERV
  if (sndserver)
    {
      /* Send a "quit" command. */
      fprintf(sndserver, "q\n");
      fflush(sndserver);
    }
#endif /* SNDSERV */
  
  /* Done. */
  return;
}


void I_InitSound(void)
{
#ifdef SNDSERV
  char *buffer = (char *)malloc(256);
  if (!buffer)
  	return;
 
  sprintf(buffer, "%s", sndserver_filename);
  find_in_path(&buffer, 256);
  
  /* start sound process */
  if ( !access(buffer, X_OK) )
    {
     if (strlen(sndserver_options))
      {
       strcat(buffer, " ");
       strcat(buffer, sndserver_options);
      }
      sndserver = popen(buffer, "w");
    }
  else
    fprintf(stderr, "Could not start sound server [%s]\n", buffer);
  
  free(buffer);
#endif /* SNDSERV */
}

void find_in_path(char **filename, int size)
{
    (void)filename;
    (void)size;
}      

