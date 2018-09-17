
/* D_main.c */

#ifdef UNIX
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "doomdef.h"
#include "p_local.h"
#ifdef USE_GSI
 #include "gsisound/soundst.h"
#else
 #include "soundclient/soundst.h"
#endif /* USE_GSI */

#endif /* UNIX */

boolean shareware = false;		/* true if only episode 1 present */
boolean ExtendedWAD = false;	        /* true if episodes 4 and 5 present */

boolean nomonsters;			/* checkparm of -nomonsters */
boolean respawnparm;			/* checkparm of -respawn */
boolean debugmode;			/* checkparm of -debug */
boolean ravpic;				/* checkparm of -ravpic */
boolean cdrom;				/* true if cd-rom mode active */
boolean singletics;			/* debug flag to cancel adaptiveness */
boolean noartiskip;			/* whether shift-enter skips an artifact */

skill_t startskill;
int startepisode;
int startmap;
boolean autostart;
extern boolean automapactive;

int lifilter=0;
int bilifilter=0;

extern char* basedefault;
extern char* homedir;


boolean advancedemo;

FILE *debugfile;

void D_CheckNetGame(void);
void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void D_DoAdvanceDemo(void);
void D_PageDrawer (void);
void D_AdvanceDemo (void);
void F_Drawer(void);
boolean F_Responder(event_t *ev);


/*
  //---------------------------------------------------------------------------
  //
  // FUNC FixedDiv, helper FixedDiv2
  //
  // FUNC FixedMul
  //
  //---------------------------------------------------------------------------
*/
fixed_t FixedDiv2( fixed_t a, fixed_t b )
{
  double c;
  
  c = ((double)a) / ((double)b) * FRACUNIT;
  
  if (c >= 2147483648.0 || c < -2147483648.0)
    I_Error("FixedDiv: divide by zero");
  return (fixed_t) c;
}

fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  if((abs(a)>>14) >= abs(b))
    {
      return((a^b)<0 ? MININT : MAXINT);
    }
  return(FixedDiv2(a, b));
}

fixed_t FixedMul( fixed_t a, fixed_t b )
{
  return ((long long) a * (long long) b) >> FRACBITS;
}

/*
  ===============================================================================
  
  EVENT HANDLING
  
  Events are asyncronous inputs generally generated by the game user.
  
  Events can be discarded if no responder claims them
  
  ===============================================================================
*/

event_t events[MAXEVENTS];
int eventhead;
int eventtail;


/*
  //---------------------------------------------------------------------------
  //
  // PROC D_PostEvent
  //
  // Called by the I/O functions when input is detected.
  //
  //---------------------------------------------------------------------------
*/
void D_PostEvent(event_t *ev)
{
  events[eventhead] = *ev;
  eventhead = (eventhead+1)&(MAXEVENTS-1);
}


/*
  //---------------------------------------------------------------------------
  //
  // PROC D_ProcessEvents
  //
  // Send all the events of the given timestamp down the responder chain.
  //
  //---------------------------------------------------------------------------
*/
void D_ProcessEvents(void)
{
  event_t *ev;
  
  for(; eventtail != eventhead; eventtail = (eventtail+1)&(MAXEVENTS-1))
    {
      ev = &events[eventtail];
      if(F_Responder(ev))
	{
	  continue;
	}
      if(MN_Responder(ev))
	{
	  continue;
	}
      G_Responder(ev);
    }
}


/*
  //---------------------------------------------------------------------------
  //
  // PROC DrawMessage
  //
  //---------------------------------------------------------------------------
*/
void DrawMessage(void)
{
  player_t *player;
  
  player = &players[consoleplayer];
  if(player->messageTics <= 0 || !player->message)
    { /* No message */
      return;
    }
  MN_DrTextA(player->message, 160-MN_TextAWidth(player->message)/2, 1);
}


/*
  //---------------------------------------------------------------------------
  //
  // PROC D_Display
  //
  // Draw current display, possibly wiping it from the previous.
  //
  //---------------------------------------------------------------------------
*/
void R_ExecuteSetViewSize(void);

extern boolean finalestage;

void D_Display(void)
{
  extern boolean MenuActive;
  extern boolean askforquit;
  
  /* Change the view size if needed */
  if(setsizeneeded)    
    R_ExecuteSetViewSize();
  
  
  /*
   * do buffered drawing
   */
  switch (gamestate)
    {
    case GS_LEVEL:
      if (!gametic)
	break;
      if (automapactive)
	AM_Drawer ();
      else
	R_RenderPlayerView (&players[displayplayer]);
      /* UpdateState |= I_FULLVIEW; */
      SB_Drawer();
      break;
    case GS_INTERMISSION:
      IN_Drawer ();
      break;
    case GS_FINALE:
      F_Drawer ();
      break;
    case GS_DEMOSCREEN:
      D_PageDrawer ();
      break;
    }
  
  if(paused && !MenuActive && !askforquit)
    {
      if(!netgame)
	{
	  V_DrawPatch(160, viewwindowy+5, W_CacheLumpName("PAUSED",
							  PU_CACHE));
	}
      else
	{
	  V_DrawPatch(160, 70+Y_DISP, W_CacheLumpName("PAUSED",
					       PU_CACHE));
	}
    }
  /* Handle player messages */
  DrawMessage();
  
  /* Menu drawing */
  MN_Drawer();
  
  /* Send out any new accumulation */
  NetUpdate();
  
  /* Flush buffered stuff to screen */
  I_FinishUpdate();

}

/*
  //---------------------------------------------------------------------------
  //
  // PROC D_DoomLoop
  //
  //---------------------------------------------------------------------------
*/
void D_DoomLoop(void)
{
  if(M_CheckParm("-debugfile"))
    {
      char filename[20];
      sprintf(filename, "debug%i.txt", consoleplayer);
      debugfile = fopen(filename,"w");
    }

  I_InitGraphics();
  
  I_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));
  
  while(1)
    {
      /* Frame syncronous IO operations */
      I_StartFrame();
      
      /* Process one or more tics */
      if(singletics)
	{
	  I_StartTic();
	  D_ProcessEvents();
	  G_BuildTiccmd(&netcmds[consoleplayer][maketic%BACKUPTICS]);
	  if (advancedemo)
	    D_DoAdvanceDemo ();
	  G_Ticker();
	  gametic++;
	  maketic++;
	  printf("SINGLE\n");
	}
      else
	{
	  /* Will run at least one tic */
	  TryRunTics();
	}
      
      /* Move positional sounds */
      S_UpdateSounds(players[consoleplayer].mo);
      D_Display();
    }
}

/*
  ===============================================================================
  
  DEMO LOOP
  
  ===============================================================================
*/

int             demosequence;
int             pagetic;
char            *pagename;


/*
  ================
  =
  = D_PageTicker
  =
  = Handles timing for warped projection
  =
  ================
*/

void D_PageTicker (void)
{
  if (--pagetic < 0)
    D_AdvanceDemo ();
}


/*
  ================
  =
  = D_PageDrawer
  =
  ================
*/

extern boolean MenuActive;

void D_PageDrawer(void)
{
  V_DrawRawScreen(W_CacheLumpName(pagename, PU_CACHE));
  if(demosequence == 1)
    {
      V_DrawPatch(4, Y_DISP+160, W_CacheLumpName("ADVISOR", PU_CACHE));
    }
  /* UpdateState |= I_FULLSCRN; */
}

/*
  =================
  =
  = D_AdvanceDemo
  =
  = Called after each demo or intro demosequence finishes
  =================
*/

void D_AdvanceDemo (void)
{
  advancedemo = true;
}

void D_DoAdvanceDemo (void)
{
  players[consoleplayer].playerstate = PST_LIVE;  /* don't reborn */
  advancedemo = false;
  usergame = false;                               /* can't save / end game here */
  paused = false;
  gameaction = ga_nothing;
  demosequence = (demosequence+1)%7;
  switch (demosequence)
    {
    case 0:
      pagetic = 210;
      gamestate = GS_DEMOSCREEN;
      pagename = "TITLE";
      
      break;
    case 1:
      pagetic = 140;
      gamestate = GS_DEMOSCREEN;
      pagename = "TITLE";
      break;
    case 2:
      BorderNeedRefresh = true;
      /*UpdateState |= I_FULLSCRN; */
      G_DeferedPlayDemo ("demo1");
      break;
    case 3:
      pagetic = 200;
      gamestate = GS_DEMOSCREEN;
      pagename = "CREDIT";
      break;
    case 4:
      BorderNeedRefresh = true;
      /* UpdateState |= I_FULLSCRN; */
      G_DeferedPlayDemo ("demo2");
      break;
    case 5:
      pagetic = 200;
      gamestate = GS_DEMOSCREEN;
      if(shareware)
	{
	  pagename = "ORDER";
	}
      else
	{
	  pagename = "CREDIT";
	}
      break;
    case 6:
      BorderNeedRefresh = true;
      /* UpdateState |= I_FULLSCRN; */
      G_DeferedPlayDemo ("demo3");
      break;
    }
}


/*
  =================
  =
  = D_StartTitle
  =
  =================
*/

void D_StartTitle (void)
{
  gameaction = ga_nothing;
  demosequence = -1;
  D_AdvanceDemo ();
}


/*
  ==============
  =
  = D_CheckRecordFrom
  =
  = -recordfrom <savegame num> <demoname>
  ==============
*/

void D_CheckRecordFrom (void)
{
  int     p;
  char    file[256];
  
  p = M_CheckParm ("-recordfrom");
  if (!p || p > myargc-2)
    return;
  
  if(cdrom)
    {
      sprintf(file, SAVEGAMENAMECD"%c.hsg",myargv[p+1][0]);
    }
  else
    {
      sprintf(file, "%s"SAVEGAMENAME"%c.hsg", homedir, myargv[p+1][0]);
    }
  G_LoadGame (file);
  G_DoLoadGame ();      /* load the gameskill etc info from savegame */
  
  G_RecordDemo (gameskill, 1, gameepisode, gamemap, myargv[p+2]);
  D_DoomLoop ();        /* never returns */
}

/*
  ===============
  =
  = D_AddFile
  =
  ===============
*/

#define MAXWADFILES 20

/*
 * MAPDIR should be defined as the directory that holds development maps
 * for the -wart # # command
 */

#ifdef __NeXT__

#define MAPDIR "/Novell/Heretic/data/"

#define SHAREWAREWADNAME "/Novell/Heretic/source/heretic1.wad"

char *wadfiles[MAXWADFILES] =
{
  "/Novell/Heretic/source/heretic.wad",
  "/Novell/Heretic/data/texture1.lmp",
  "/Novell/Heretic/data/texture2.lmp",
  "/Novell/Heretic/data/pnames.lmp"
};

#else

#define MAPDIR "./"

#define SHAREWAREWADNAME "heretic_share.wad"

char *wadfiles[MAXWADFILES] =
{
    "heretic.wad"
#ifdef EXTRA_WADS
    ,
    "texture1.lmp",
    "texture2.lmp",
    "pnames.lmp"
#endif     
};
 
#endif

char exrnwads[80];
char exrnwads2[80];

void wadprintf(void)
{
  if(debugmode)
    {
      return;
    }  
}

void D_AddFile(char *file)
{
  int numwadfiles;
  char *newf;
  /*	char text[256]; */
  
  for(numwadfiles = 0; wadfiles[numwadfiles]; numwadfiles++);
  newf = malloc(strlen(file)+1);
  assert(newf);
  strcpy(newf, file);
  if(strlen(exrnwads)+strlen(file) < 78)
    {
      if(strlen(exrnwads))
	{
	  strcat(exrnwads, ", ");
	}
      else
	{
	  strcpy(exrnwads, "External Wadfiles: ");
	}
      strcat(exrnwads, file);
    }
  else if(strlen(exrnwads2)+strlen(file) < 79)
    {
      if(strlen(exrnwads2))
	{
	  strcat(exrnwads2, ", ");
	}
      else
	{
	  strcpy(exrnwads2, "     ");
	  strcat(exrnwads, ",");
	}
      strcat(exrnwads2, file);
    }
  wadfiles[numwadfiles] = newf;
}


/*
  //---------------------------------------------------------------------------
  //
  // PROC D_DoomMain
  //
  //---------------------------------------------------------------------------
*/
void D_DoomMain(void)
{
  int p;
  int e;
  int m;
  char file[256];
  
  /* FILE *fp; */
  int fd;
  
  boolean devMap;
  /*   char *screen;   */
  /*   char *startup;   */
  /*   char smsg[80];   */
  
  
  printf("========================================================\n");
  printf("==                                                    ==\n");
  printf("==                 HERETIC v1.0.3                     ==\n");
  printf("==                                                    ==\n");
  printf("==         for GGI, X11, SDL and SVGAlib              ==\n");
  printf("==                   works on                         ==\n");
  printf("==      x86, m68k, Alpha and Netwinder-systems        ==\n");
  printf("==      under Linux, FreeBSD, SCO-Unix (tested)       ==\n");
  printf("==                                                    ==\n");
  printf("==   Heretic was ported to LINUX by Andre Werthmann   ==\n");
  printf("==    You can download the latest versions under:     ==\n");
  printf("==      http://www.raven-games.com/linuxheretic       ==\n");
  printf("==                                                    ==\n");
  printf("==                                                    ==\n");
  printf("==              Press Return to go on.                ==\n");
  printf("==                                                    ==\n");
  printf("========================================================\n");
  /* getchar(); */

  /* calls SVGALib init and revokes root rights, dummy for other displays */
  InitGraphLib();
  
  /* Gets the home-directory of the user */
  I_GetHomeDirectory();
  
  fprintf(stdout, "Using homedirectory for savegames: %s\n", homedir);
  
  /* fprintf(stdout, "basedefault: %s\n", basedefault ); */
  
  /* Add current directory to path */
  if (getenv("PATH"))
  {
  	char *path = getenv("PATH");
  	char *newpath = malloc(strlen(path)+2);
  	sprintf(newpath, "%s:", path);
  	setenv("PATH", newpath, 1);
  	free(newpath);
  }
  
  /* Check if filtering is wanted */ 
  if ( M_CheckParm("-linear")!=0 ) lifilter=1;
  if ( M_CheckParm("-bilinear")!=0 ) bilifilter=1;

  M_FindResponseFile();
  setbuf(stdout, NULL);
  nomonsters = M_CheckParm("-nomonsters");
  respawnparm = M_CheckParm("-respawn");
  ravpic = M_CheckParm("-ravpic");
  noartiskip = M_CheckParm("-noartiskip");
  debugmode = M_CheckParm("-debug");
  startskill = sk_medium;
  startepisode = 1;
  startmap = 1;
  autostart = false;
  
  /* wadfiles[0] is a char * to the main wad */
  
  /*
   * fp = fopen(wadfiles[0], "rb");
   * if(fp)
   *  {
   *    fclose(fp);
   *  }
   */
  
  fd = wadopen(wadfiles[0]);
  if (fd >= 0)
      {
	  close(fd);
      }
  
  else
      { /* Change to look for shareware wad */
	  printf("Full-version HereticWAD not found ! defaulting to shareware WAD...\n\n");
	  wadfiles[0] = SHAREWAREWADNAME;
      }
  
  /* Check for -CDROM */
  cdrom = false;  

  /*
   * -FILE [filename] [filename] ...
   * Add files to the wad list.
   */
  p = M_CheckParm("-file");
  if(p)
    {	
      /*
       * the parms after p are wadfile/lump names, until end of parms
       * or another - preceded parm
       */
      while(++p != myargc && myargv[p][0] != '-')
	{
	  D_AddFile(myargv[p]);
	}
    }
  /*
   * -DEVMAP <episode> <map>
   * Adds a map wad from the development directory to the wad list,
   * and sets the start episode and the start map.
   */
  devMap = false;
  p = M_CheckParm("-devmap");
  if(p && p < myargc-2)
    {
      e = myargv[p+1][0];
      m = myargv[p+2][0];
      sprintf(file, MAPDIR"E%cM%c.wad", e, m);
      D_AddFile(file);
      printf("DEVMAP: Episode %c, Map %c.\n", e, m);
      startepisode = e-'0';
      startmap = m-'0';
      autostart = true;
      devMap = true;
    }
  
  p = M_CheckParm("-playdemo");
  if(!p)
    {
      p = M_CheckParm("-timedemo");
    }
  if (p && p < myargc-1)
    {
      sprintf(file, "%s.lmp", myargv[p+1]);
      D_AddFile(file);
      printf("Playing demo %s.lmp.\n", myargv[p+1]);
    }
  
  /*
   * get skill / episode / map from parms
   */
  if(M_CheckParm("-deathmatch"))
    {
      deathmatch = true;
    }
  
  p = M_CheckParm("-skill");
  if(p && p < myargc-1)
    {
      startskill = myargv[p+1][0]-'1';
      autostart = true;
    }
  
  p = M_CheckParm("-episode");
  if(p && p < myargc-1)
    {
      startepisode = myargv[p+1][0]-'0';
      startmap = 1;
      autostart = true;
    }
  
  p = M_CheckParm("-warp");
  if(p && p < myargc-2)
    {
      startepisode = myargv[p+1][0]-'0';
      startmap = myargv[p+2][0]-'0';
      autostart = true;
    }
  
  /*
   * init subsystems
   */
  printf("V_Init: allocate screens.\n");
  V_Init();
  
  /* Load defaults before initing other systems */
  printf("M_LoadDefaults: Load system defaults.\n");
  M_LoadDefaults();
  
  printf("Z_Init: Init zone memory allocation daemon.\n");
  Z_Init();
  
  printf("W_Init: Init WADfiles.\n");
  W_InitMultipleFiles(wadfiles);
  
  if(W_CheckNumForName("E2M1") == -1)
    { /* Can't find episode 2 maps, must be the shareware WAD */
      shareware = true;
    }
  else if(W_CheckNumForName("EXTENDED") != -1)
    { /* Found extended lump, must be the extended WAD */
      ExtendedWAD = true;
    }
  
  /*   startup = W_CacheLumpName("LOADING", PU_CACHE);   */
  
  /*
   *  Build status bar line!
   */
  /*   smsg[0] = 0;   */
  if (deathmatch)
    printf("DeathMatch...\n");
  if (nomonsters)
    printf("No Monsters...\n");
  if (respawnparm)
    printf("Respawning...\n");
  if (autostart)
    {
      char temp[64];
      sprintf(temp, "Warp to Episode %d, Map %d, Skill %d ",
	      startepisode, startmap, startskill+1);
      printf("%s\n", temp);
    }
  wadprintf(); /* print the added wadfiles */
  
  printf("MN_Init: Init menu system.\n");
  MN_Init();
  
  printf("R_Init: Init Heretic refresh daemon.\n");
  
  printf("Loading graphics\n");
  R_Init();
  
  printf("\nP_Init: Init Playloop state.\n");
  
  printf("Init game engine.\n");
  P_Init();
  
  printf("I_Init: Setting up machine state.\n");
  I_Init();
  
  printf("D_CheckNetGame: Checking network game status.\n");
  
  printf("Checking network game status.\n");
  D_CheckNetGame();
  
  printf("S_Init: Setting up sound.\n");
  S_Init( snd_SfxVolume, 10 ); 
 
  printf("SB_Init: Loading patches.\n");
  SB_Init();

  /*
   * start the apropriate game based on parms
   */
  
  D_CheckRecordFrom();
  
  p = M_CheckParm("-record");
  if(p && p < myargc-1)
    {
      G_RecordDemo(startskill, 1, startepisode, startmap, myargv[p+1]);
      D_DoomLoop(); /* Never returns */
    }
  
  p = M_CheckParm("-playdemo");
  if(p && p < myargc-1)
    {
      singledemo = true; /* Quit after one demo */
      G_DeferedPlayDemo(myargv[p+1]);
      D_DoomLoop(); /* Never returns */
    }
  
  p = M_CheckParm("-timedemo");
  if(p && p < myargc-1)
    {
      G_TimeDemo(myargv[p+1]);
      D_DoomLoop(); /* Never returns */
    }
  
  p = M_CheckParm("-loadgame");
  if(p && p < myargc-1)
    {
      if(cdrom)
	{
	  sprintf(file, SAVEGAMENAMECD"%c.hsg", myargv[p+1][0]);
	}
      else
	{
	  sprintf(file, "%s"SAVEGAMENAME"%c.hsg", homedir, myargv[p+1][0]);
	}
      G_LoadGame(file);
    }
  
  /* Check valid episode and map */
  if((autostart || netgame) && (devMap == false))
    {
      if(M_ValidEpisodeMap(startepisode, startmap) == false)
	{
	  startepisode = 1;
	  startmap = 1;
	}
    }
  
  if(gameaction != ga_loadgame)
    {
      /* UpdateState |= I_FULLSCRN; */
      BorderNeedRefresh = true;
      if(autostart || netgame)
	{
	  G_InitNew(startskill, startepisode, startmap);
	}
      else
	{
	  D_StartTitle();
	}
    }
  D_DoomLoop(); /* Never returns */
}


