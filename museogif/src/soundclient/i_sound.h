#ifndef __SOUND__
#define __SOUND__

#include "doomdef.h"
#include "sounds.h"

/* UNIX hack, to be removed. */
#ifdef SNDSERV
#include <stdio.h>
extern FILE* sndserver;
extern char* sndserver_filename;
extern char* sndserver_options;
#endif

#ifdef MUSSERV
extern char *musserver_filename;
extern char *musserver_options;
#endif

extern long snd_SfxVolume;
extern long snd_MusicVolume;



/* Init at program start... */
void I_InitSound(void);

/* ... update sound buffer and audio device at runtime... */
void I_UpdateSound(void);
void I_SubmitSound(void);

/* ... shut down and relase at program termination. */
void I_ShutdownSound(void);


/*
 *  SFX I/O
 */

/* Initialize channels? */
void I_SetChannels(void);

/* Get raw data lump index for sound descriptor. */
int I_GetSfxLumpNum (sfxinfo_t* sfxinfo );


/* Starts a sound in a particular sound channel. */
int I_StartSound( 
		 int		id,
		 int		vol,
		 int		sep,
		 int		pitch,
		 int		priority 
		 );


/* Stops a sound channel. */
void I_StopSound(int handle);

/*
 * Called by S_*() functions
 *  to see if a channel is still playing.
 * Returns 0 if no longer playing, 1 if playing.
 */
int I_SoundIsPlaying(int handle);

/*
 * Updates the volume, separation,
 *  and pitch of a sound channel.
 */
void I_UpdateSoundParams( 
			 int		handle,
			 int		vol,
			 int		sep,
			 int		pitch 
			 );


/*
 *  MUSIC I/O
 */
void I_InitMusic(void);
void I_ShutdownMusic(void);

/* Volume. */
void I_SetMusicVolume(int volume);

/* PAUSE game handling. */
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

/* Registers a song handle to song data. */
int I_RegisterSong(void *data, int len);

/*
 * Called by anything that wishes to start music.
 *  plays a song, and when the song is done,
 *  starts playing it again in an endless loop.
 * Horrible thing to do, considering.
 */
void I_PlaySong( 
		int		handle,
		int		looping 
		);
/* Stops a song over 3 seconds. */
void I_StopSong(int handle);

/* See above (register), then think backwards */
void I_UnRegisterSong(int handle);

/* Find an executable in the path (for finding servers) */
void find_in_path(char **filename, int size);

#endif
