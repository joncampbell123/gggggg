
#ifndef __SOUNDSTH__
#define __SOUNDSTH__


extern int snd_SfxVolume;


/*
 * Initializes sound stuff, including volume
 * Sets channels, SFX and music volume,
 *  allocates channel buffer, sets S_sfx lookup.
 */
void S_Init( int sfxVolume, int musicVolume );


/*
 * Per level startup code.
 * Kills playing sounds at start of level,
 *  determines music if any, changes music.
 */
void S_Start(void);


/*
 * Start sound for thing at <origin>
 *  using <sound_id> from sounds.h
 */
void S_StartSound( void* origin, int sound_id );


/* Will start a sound at a given volume. */
void S_StartSoundAtVolume( void* origin, int sound_id, int volume );


/* Stop sound for thing at <origin> */
void S_StopSound(void* origin);


/* Stop and resume music, during game PAUSE. */
void S_PauseSound(void);
void S_ResumeSound(void);


/*
 * Updates music & sounds
 */
void S_UpdateSounds(void* listener);

void S_SetSfxVolume(int volume);


#endif /* __SOUNDSTH__ */
