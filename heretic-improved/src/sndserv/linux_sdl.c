/*
 *-----------------------------------------------------------------------------
 *
 * $Log: linux.c,v $
 *
 * Revision 2.0  1999/01/24 00:52:12  Andre Wertmann
 * changed to work with Heretic 
 *
 * Revision 1.4  1998/06/23 12:21:38  rafaj
 * included missing <sys/ioctl.h>
 *
 * Revision 1.3  1997/01/26 07:45:01  b1
 * 2nd formatting run, fixed a few warnings as well.
 *
 * Revision 1.2  1997/01/21 19:00:01  b1
 * First formatting run:
 *  using Emacs cc-mode.el indentation for C++ now.
 *
 * Revision 1.1  1997/01/19 17:22:45  b1
 * Initial check in DOOM sources as of Jan. 10th, 1997
 *
 *
 * DESCRIPTION:
 *	UNIX, soundserver for Linux i386.
 *
 *-----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <SDL/SDL.h>

#include "soundsrv.h"

static SDL_AudioSpec	SDL_audio;
static int16_t*		outbuffer=NULL;
static unsigned int	outbuffer_in=0,outbuffer_out=0,outbuffer_max=0;

static void audio_cb(void __attribute__((unused)) *user,uint8_t *stream,int len) {
	int16_t *out = (int16_t*)stream;
	len /= 4;

	while (len > 0) {
		if (outbuffer_out == outbuffer_in)
			break;

		*out++ = outbuffer[(outbuffer_out*2)  ];
		*out++ = outbuffer[(outbuffer_out*2)+1];
		if ((++outbuffer_out) >= outbuffer_max) outbuffer_out = 0;
		len--;
	}

	while (len-- > 0) {
		*out++ = 0;
		*out++ = 0;
	}
}

void I_InitMusic(void)
{
}

void I_InitSound( int samplerate, int samplesize )
{
  memset(&SDL_audio,0,sizeof(SDL_audio));

  fprintf(stderr,"I_InitSound SDL\n");
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
	  fprintf(stderr,"Failed to init SDL audio\n");
	  return;
  }

  outbuffer_in=0;
  outbuffer_out=0;
  SDL_audio.freq = 11025;
  SDL_audio.format = AUDIO_S16;
  SDL_audio.channels = 2;
  SDL_audio.silence = 0;
  SDL_audio.samples = 512;
  SDL_audio.size = 2 * 2 * 1024;
  SDL_audio.callback = audio_cb;

  if (SDL_OpenAudio(&SDL_audio,NULL)) {
          memset(&SDL_audio,0,sizeof(SDL_audio));
	  fprintf(stderr,"Failed to open SDL audio\n");
	  return;
  }

  outbuffer_max = SDL_audio.samples * 2;
  outbuffer = malloc(4 * outbuffer_max);
  if (outbuffer == NULL) {
	  fprintf(stderr,"Failed to alloc for SDL\n");
	  abort();
  }

  SDL_PauseAudio(0/*unpause*/);
}

void I_SubmitOutputBuffer( void* samples, int samplecount )
{
  int16_t *i = (int16_t*)samples,*o;
  unsigned int cc;

  if (SDL_audio.size == 0) {
          usleep(10000);
	  return;
  }

  SDL_LockAudio();
  while (samplecount > 0) {
    cc = outbuffer_in;
    if ((++cc) >= outbuffer_max) cc = 0;
    if (cc == outbuffer_out) {
	    /* calling thread expects us to block in this case, so.. */
            SDL_UnlockAudio();
	    usleep(1000);
	    SDL_LockAudio();
	    continue;
    }

    o = outbuffer + (outbuffer_in*2);
    outbuffer_in = cc;
    samplecount--;
    *o++ = *i++;
    *o++ = *i++;
  }
  SDL_UnlockAudio();
}

void I_ShutdownSound(void)
{
  SDL_CloseAudio();
  if (outbuffer) free(outbuffer);
  outbuffer = NULL;
}

void I_ShutdownMusic(void)
{
}
