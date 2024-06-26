
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h> /* for lseek, open */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gsi/gsi.h>

#include "i_sound.h"
#include "sounds.h"
#include "soundst.h"

#include "doomdef.h"
#include "r_local.h" /* For R_PointToAngle2 */


/* Purpose? */
const char snd_prefixen[]
= { 'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S', 'S', 'S' };

extern char *doomwaddir; /* for GSI */

#define S_MAX_VOLUME		127

/*
 * when to clip out sounds
 * Does not fit the large outdoor areas.
 */
#define S_CLIPPING_DIST		(1200*0x10000)

/*
 * Distance tp origin when sounds should be maxed out.
 * This should relate to movement clipping resolution
 * (see BLOCKMAP handling).
 * Originally: (200*0x10000).
 */
#define S_CLOSE_DIST		(160*0x10000)


#define S_ATTENUATOR		((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

/* Adjustable by menu. */
#define NORM_VOLUME    		snd_MaxVolume

#define NORM_PITCH     		128
#define NORM_PRIORITY		64
#define NORM_SEP		128

#define S_PITCH_PERTURB		1
#define S_STEREO_SWING		(96*0x10000)

/* percent attenuation from front to back */
#define S_IFRACVOL		30

#define NA			0
#define S_NUMCHANNELS		2


/*
 * Current music/sfx card - index useless
 *  w/o a reference LUT in a sound module.
 */
extern int snd_SfxDevice;
/* Config file? Same disclaimer as above. */
extern int snd_DesiredSfxDevice;


typedef struct
{
    /* sound information (if null, channel avail.) */
    sfxinfo_t*	sfxinfo;

    /* origin of sound */
    void*	origin;

    /* handle of the sound being played */
    int		handle;
    
} channel_t;


/* the set of channels available */
static channel_t*	channels;

/*
 * These are not used, but should be (menu).
 * Maximum volume of a sound effect.
 * Internal default is max out of 0-15.
 */
int 		snd_SfxVolume = 15;


/* whether songs are mus_paused */
static boolean		mus_paused;	

/* music currently being played */
static musicinfo_t*	mus_playing=0;

/*
 * following is set
 *  by the defaults code in M_misc:
 * number of channels available
 */
int			numChannels;	

static int		nextcleanup;



/*
 * Internals.
 */
int S_getChannel( void* origin, sfxinfo_t* sfxinfo );


int S_AdjustSoundParams( mobj_t* listener, mobj_t* source, 
			 int* vol, int* sep, int* pitch );

void S_StopChannel(int cnum);


extern char* wadfiles[];

/*
 * Initializes sound stuff, including volume
 * Sets channels, SFX and music volume,
 *  allocates channel buffer, sets S_sfx lookup.
 */
void S_Init( int sfxVolume, int	musicVolume )
{
    int		i;
    
    fprintf( stderr, "S_Init: default sfx volume %d music volume %d\n", sfxVolume, musicVolume);
    
    /* Whatever these did with DMX, these are rather dummies now. */
    I_SetChannels();
    
    S_SetSfxVolume(sfxVolume);
    
    numChannels = 8; /* whs: added .. */
    /*
     * Allocating the internal channels for mixing
     * (the maximum numer of sounds rendered
     * simultaneously) within zone memory.
     */
    channels =
	(channel_t *) Z_Malloc(numChannels*sizeof(channel_t), PU_STATIC, 0);
    
    /* Free all channels for use */
    for (i=0 ; i<numChannels ; i++)
	channels[i].sfxinfo = 0;
    
    /* no sounds are playing, and they are not mus_paused */
    mus_paused = 0;
    
    /* Note that sounds have not been cached (yet). */
    for (i=1 ; i<NUMSFX ; i++)
	S_sfx[i].usefulness = -1;
    /* BAD!    S_sfx[i].lumpnum = S_sfx[i].usefulness = -1; */
    
    /* WHS: instrument loading code for GSI */
    {
	unsigned char OPL2_genmidi[32];   /* GM instruments read from doom.wad */
	unsigned char OPL2_operators[22]; /* sent to gsi_server */
	int  genmidi_lumpno;
	int  fd;
	
	fprintf(stderr, "DOOM: OPL instrument loading\n");
	genmidi_lumpno = W_GetNumForName("GENMIDI");
	fprintf(stderr, "DOOM: GENMIDI lumpno=%d, position = %d, length = %d\n", genmidi_lumpno, lumpinfo[genmidi_lumpno].position, W_LumpLength(genmidi_lumpno));
	


	
	/* Andre: this should be fixed soon. */
	chdir(doomwaddir);
	fd = open(wadfiles[0], O_RDONLY);
	
	


	lseek(fd, lumpinfo[genmidi_lumpno].position + 8, SEEK_SET);
	/* 8 skips the #OPL_II# signature */
	for (i=0; i<175; i++) {
	    int  j;
	    struct opl_instr {
		unsigned short   flags;
		unsigned char    finetune;
		unsigned char    note;
		/* sbi_instr_data   instrument_data; */
		unsigned char    instrument_data[32];
	    };
	    struct opl_instr doom_opl_instr;
	    
	    /* read OPL2_genmidi */
	    read(fd, &doom_opl_instr, sizeof(struct opl_instr));
	    memcpy(OPL2_genmidi, doom_opl_instr.instrument_data, 32);
	    
	    if (i>=128) j=i+35; else j=i;
	    
	    /* this is to use the std.o3/std.sb instead of doom's OPL_II instr's: */
	    if (i<128) continue;
	    
	    OPL2_operators[ 0] = OPL2_genmidi[ 0];
	    OPL2_operators[ 1] = OPL2_genmidi[ 7];
	    OPL2_operators[ 2] = OPL2_genmidi[ 4] + OPL2_genmidi[ 5];
	    OPL2_operators[ 3] = OPL2_genmidi[11] + OPL2_genmidi[12];
	    OPL2_operators[ 4] = OPL2_genmidi[ 1];
	    OPL2_operators[ 5] = OPL2_genmidi[ 8];
	    OPL2_operators[ 6] = OPL2_genmidi[ 2];
	    OPL2_operators[ 7] = OPL2_genmidi[ 9];
	    OPL2_operators[ 8] = OPL2_genmidi[ 3];
	    OPL2_operators[ 9] = OPL2_genmidi[10];
	    OPL2_operators[10] = OPL2_genmidi[ 6];
	    
	    gsi_send_commands(3, GSI_CMD_LOAD_INSTRUMENT_DATA, j, GSI_INSTRUMENT_OPL2);
	    gsi_send_int32(11);
	    gsi_send_block(OPL2_operators, 11);
	}
    }
    gsi_flush(); /* not really needed. */
}


/*
 * Per level startup code.
 * Kills playing sounds at start of level,
 *  determines music if any, changes music.
 */
void S_Start(void)
{
    int cnum;
    int mnum;
    
    /*
     * kill all playing sounds at start of level
     *  (trust me - a good idea)
     */
    for (cnum=0 ; cnum<numChannels ; cnum++)
	if (channels[cnum].sfxinfo)
	    S_StopChannel(cnum);
    
    /* start new music for the level */
    mus_paused = 0;     
    
    if (!shareware)
	mnum = gamemap - 1;
    
    else
	{
	    int spmus[]=
	    {
		/* Song - Who? - Where? */
		
		mus_e3m4,	/* American	e4m1 */
		mus_e3m2,	/* Romero	e4m2 */
		mus_e3m3,	/* Shawn	e4m3 */
		mus_e1m5,	/* American	e4m4 */
		mus_e2m7,	/* Tim 	e4m5 */
		mus_e2m4,	/* Romero	e4m6 */
		mus_e2m6,	/* J.Anderson	e4m7 CHIRON.WAD */
		mus_e2m5,	/* Shawn	e4m8 */
		mus_e1m9	/* Tim		e4m9 */
	    };

    if (gameepisode < 4)
      mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
    else
      mnum = spmus[gamemap-1];
    }
  
    nextcleanup = 15;
}


void S_StartSoundAtVolume( void* origin_p, int sfx_id, int volume )
{
    int		rc;
    int		sep;
    int		pitch;
    int		priority;
    sfxinfo_t*	sfx;
    int		cnum;
    
    mobj_t*	origin = (mobj_t *) origin_p;
    
    /* Debug. */
    /*fprintf( stderr,
      "S_StartSoundAtVolume: playing sound %d (%s)\n",
      sfx_id, S_sfx[sfx_id].name );*/
    
    /* check for bogus sound # */
    if (sfx_id < 1 || sfx_id > NUMSFX)
	I_Error("Bad sfx #: %d", sfx_id);
    
    sfx = &S_sfx[sfx_id];
    
    /* Initialize sound parameters */
    if (sfx->link)
	{
	    pitch = sfx->pitch;
	    priority = sfx->priority;
	    volume += sfx->volume;
	    
	    if (volume < 1)
		return;
	    
	    if (volume > snd_SfxVolume)
		volume = snd_SfxVolume;
	}
    else
	{
	    pitch = NORM_PITCH;
	    priority = NORM_PRIORITY;
	}
    
    /*
     * Check to see if it is audible,
     *  and if not, modify the params
     */
    if (origin && origin != players[consoleplayer].mo)
	{
	    rc = S_AdjustSoundParams(players[consoleplayer].mo,
				     origin,
				     &volume,
				     &sep,
				     &pitch);
	    
	    if ( origin->x == players[consoleplayer].mo->x
		 && origin->y == players[consoleplayer].mo->y)
		{
		    sep 	= NORM_SEP;
		}
	    
	    if (!rc)
		return;
	}
    else
	{
	    sep = NORM_SEP;
	}
    
    /* needed: without this pitches are the same for all sounds->ugly */
    /* hacks to vary the sfx pitches */
    if (sfx_id >= sfx_gntact
	&& sfx_id <= sfx_phohit)
	{
	    pitch += 8 - (M_Random()&15);
	    
	    if (pitch<0)
		pitch = 0;
	    else if (pitch>255)
		pitch = 255;
	}
    else if (sfx_id != sfx_itemup
	     && sfx_id != sfx_stnmov)
	{
	    pitch += 16 - (M_Random()&31);
	    
	    if (pitch<0)
		pitch = 0;
	    else if (pitch>255)
		pitch = 255;
	}
    
    /* kill old sound */
    S_StopSound(origin);
    
    /* try to find a channel */
    cnum = S_getChannel(origin, sfx);
    
  if (cnum<0)
      return;
  
  /*
   * This is supposed to handle the loading/caching.
   * For some odd reason, the caching is done nearly
   *  each time the sound is needed?
   */
  
  /* get lumpnum if necessary */
  if (sfx->lumpnum < 0) {
      if (sfx->link)
	  sfx->lumpnum = I_GetSfxLumpNum(sfx->link);
      else
	  sfx->lumpnum = I_GetSfxLumpNum(sfx);
  }
  
  /* increase the usefulness */
  if (sfx->usefulness++ < 0)
      sfx->usefulness = 1;
  
  /*
   * Assigns the handle to one of the channels in the
   *  mix/output buffer.
   */
  channels[cnum].handle = I_StartSound(sfx_id,
				       /*sfx->data,*/
				       volume,
				       sep,
				       pitch,
				       priority);
}


void S_StartSound( void* origin, int sfx_id )
{  
    S_StartSoundAtVolume(origin, sfx_id, snd_SfxVolume);
}


void S_StopSound(void *origin)
{
    
    int cnum;
    
    for (cnum=0 ; cnum<numChannels ; cnum++)
	{
	    if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
		{
		    S_StopChannel(cnum);
		    break;
		}
	}
}


/*
 * Stop and resume music, during game PAUSE.
 */
void S_PauseSound(void)
{
    if (mus_playing && !mus_paused)
	{
	    I_PauseSong(mus_playing->handle);
	    mus_paused = true;
	}
}

void S_ResumeSound(void)
{
    if (mus_playing && mus_paused)
	{
	    I_ResumeSong(mus_playing->handle);
	    mus_paused = false;
	}
}


/*
 * Updates music & sounds
 */
void S_UpdateSounds(void* listener_p)
{
    int		audible;
    int		cnum;
    int		volume;
    int		sep;
    int		pitch;
    sfxinfo_t*	sfx;
    channel_t*	c;
    
    mobj_t*	listener = (mobj_t*)listener_p;
    
    for (cnum=0 ; cnum<numChannels ; cnum++)
	{
	c = &channels[cnum];
	sfx = c->sfxinfo;
	
	if (c->sfxinfo)
	    {
		if (I_SoundIsPlaying(c->handle))
		    {
			/* initialize parameters */
			volume = snd_SfxVolume;
			pitch = NORM_PITCH;
			sep = NORM_SEP;
			
			if (sfx->link)
			    {
				pitch = sfx->pitch;
				volume += sfx->volume;
				if (volume < 1)
				    {
					S_StopChannel(cnum);
					continue;
				    }
				else if (volume > snd_SfxVolume)
				    {
					volume = snd_SfxVolume;
				    }
			    }
			
			/*
			 * check non-local sounds for distance clipping
			 *  or modify their params
			 */
			if (c->origin && listener_p != c->origin)
		{
		    audible = S_AdjustSoundParams(listener,
						  c->origin,
						  &volume,
						  &sep,
						  &pitch);
		    
		    if (!audible)
			{
			    S_StopChannel(cnum);
			}
		    else
			I_UpdateSoundParams(c->handle, volume, sep, pitch);
		}
		    }
		else
		    {
			/*
			 * if channel is allocated but sound has stopped,
			 *  free it
			 */
			S_StopChannel(cnum);
		    }
	    }
	}    
}


void S_SetSfxVolume(int volume)
{

    if (volume < 0 || volume > 127)
	I_Error("Attempt to set sfx volume at %d", volume);

    snd_SfxVolume = volume;

}


void S_StopChannel(int cnum)
{    
    int		i;
    channel_t*	c = &channels[cnum];
    
    if (c->sfxinfo)
	{
	    /* stop the sound playing */
	    if (I_SoundIsPlaying(c->handle))
		{
		    I_StopSound(c->handle);
		}
	    
	    /*
	     * check to see
	     *  if other channels are playing the sound
	     */
	    for (i=0 ; i<numChannels ; i++)
		{
		    if (cnum != i
			&& c->sfxinfo == channels[i].sfxinfo)
			{
			    break;
			}
		}
	    
	    /* degrade usefulness of sound data */
	    c->sfxinfo->usefulness--;
	    
	    c->sfxinfo = 0;
	}
}


/*
 * Changes volume, stereo-separation, and pitch variables
 *  from the norm of a sound effect to be played.
 * If the sound is not audible, returns a 0.
 * Otherwise, modifies parameters and returns 1.
 */
int S_AdjustSoundParams( mobj_t* listener, mobj_t* source,
			 int* vol, int* sep, int* pitch )
{
    fixed_t	approx_dist;
    fixed_t	adx;
    fixed_t	ady;
    angle_t	angle;

    /*
     * calculate the distance to sound origin
     *  and clip it if necessary
     */
    adx = abs(listener->x - source->x);
    ady = abs(listener->y - source->y);

    /* From _GG1_ p.428. Appox. eucledian distance fast. */
    approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);
    
    if (gamemap != 8
	&& approx_dist > S_CLIPPING_DIST)
	{
	    return 0;
	}
    
    /* angle of source to listener */
    angle = R_PointToAngle2(listener->x,
			    listener->y,
			    source->x,
			    source->y);
    
    if (angle > listener->angle)
	angle = angle - listener->angle;
    else
	angle = angle + (0xffffffff - listener->angle);

    angle >>= ANGLETOFINESHIFT;
    
    /* stereo separation */
    *sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

    /* volume calculation */
    if (approx_dist < S_CLOSE_DIST)
	{
	    *vol = snd_SfxVolume;
	}
    else if (gamemap == 8)
	{
	    if (approx_dist > S_CLIPPING_DIST)
		approx_dist = S_CLIPPING_DIST;
	    
	    *vol = 15+ ((snd_SfxVolume-15)
			*((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
		/ S_ATTENUATOR;
	}
    else
	{
	    /* distance effect */
	    *vol = (snd_SfxVolume
		    * ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
		/ S_ATTENUATOR; 
	}
    
    return (*vol > 0);
}


/*
 * S_getChannel :
 *   If none available, return -1.  Otherwise channel #.
 */
int S_getChannel( void* origin, sfxinfo_t* sfxinfo )
{
    /* channel number to use */
    int		cnum;
    
    channel_t*	c;
    /* Find an open channel */
    for (cnum=0 ; cnum<numChannels ; cnum++)
	{
	    if (!channels[cnum].sfxinfo)
		break;
	    else if (origin &&  channels[cnum].origin ==  origin)
		{
		    S_StopChannel(cnum);
		    break;
		}
	}
    
    /* None available */
    if (cnum == numChannels)
	{
	    /* Look for lower priority */
	    for (cnum=0 ; cnum<numChannels ; cnum++)
		if (channels[cnum].sfxinfo->priority >= sfxinfo->priority) break;
	    
	    if (cnum == numChannels)
		{
		    /* FUCK!  No lower priority.  Sorry, Charlie. */
		    return -1;
		}
	    else
		{
		    /* Otherwise, kick out lower priority. */
		    S_StopChannel(cnum);
		}
	}
    
    c = &channels[cnum];
    
    /* channel is decided to be cnum. */
    c->sfxinfo = sfxinfo;
    c->origin = origin;
    
    return cnum;
}
