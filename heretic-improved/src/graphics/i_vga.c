/* SVGAlib display, keyboard and mouse code for Heretic.
 *
 * Copyright (C) Nic Bellamy <sky@wibble.net> 21/01/1999.
 *
 * 12/02/1999 - minor changes by A.Werthmann
 * 23/02/1999 - background run support by J.Rafaj
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include <vga.h>
#include <vgakeyboard.h>
#include <vgamouse.h>

#include "doomdef.h"

/* This variable is set true automatically whenever we switch back to
 * VC running Heretic. It signalises that we shouldn't update
 * the keyboard untill keys are released.
 */
volatile boolean lock_keys = 0;

/* You can change this to 1 for little dots on the screen */
boolean	devparm=0;

/* These are the character strings representing the mouse device file and
 * type. The defaults are set in m_misc.c, or read from ~/.doomrc if it exists
 */
extern char *mousedev;

/* if we have a working mouse device, this will be non-zero. */
int mouseworks = 0;
int mousestate = 0;

/* EVENT_BUFFER sets the maximum number of keyboard events that can be kept in
 * the queue - 32 should be many times more than we'll need.
 */
#define EVENT_BUFFER		(32)
#define KEY_DOWN_EVENT		(1)
#define KEY_UP_EVENT		(2)
#define MOUSE_EVENT		(3)

typedef struct {
	int	event;
	union {
		int key;
		struct {
			int button;
			int dx,dy;
		} mouse;
	} data;
} event_q;

event_q event_queue[EVENT_BUFFER];

int event_head, event_tail;

void event_put(event_q *event)
{
	memcpy(&event_queue[event_head], event, sizeof(event_q));
	event_head = (event_head + 1) % EVENT_BUFFER;
	if (event_head == event_tail) {
		I_Error("event_put: Aiee! event_queue overflow!\n");
	}
}

int event_get(event_q *event)
{
	keyboard_update();
	if (mouseworks) {
		mouse_update();
	}
	if (event_head == event_tail) {
		return 0;
	}
	memcpy(event, &event_queue[event_tail], sizeof(event_t));
	event_tail = (event_tail + 1) % EVENT_BUFFER;
	return 1;
}

void kb_event_handler(int scancode, int press)
{
	event_q current;

	if (press == KEY_EVENTPRESS) {
		current.event = KEY_DOWN_EVENT;
	}
	if (press == KEY_EVENTRELEASE) {
		current.event = KEY_UP_EVENT;
	}
	if (lock_keys) {
		
                if (press == KEY_EVENTPRESS)
                        return;
                else
                        lock_keys = 0;
	}
	current.data.key = scancode;
	event_put(&current);
}

void mouse_event_handler(int button, int dx, int dy, int dz,
	int drx, int dry, int drz)
{
	event_q current;

	current.event = MOUSE_EVENT;
	current.data.mouse.button = button;
	current.data.mouse.dx = dx;
	current.data.mouse.dy = dy;
	event_put(&current);
}

/*
 * scancode -> something-almost-like-ASCII translation
 */

static int keyxlat(int sym)
{
  switch (sym) {
  case SCANCODE_REMOVE: 
    return KEY_DELETE; 
  case SCANCODE_INSERT: 
    return KEY_INSERT; 
  case SCANCODE_PAGEUP: 
    return KEY_PAGEUP; 
  case SCANCODE_PAGEDOWN: 
    return KEY_PAGEDOWN; 
  case SCANCODE_HOME: 
    return KEY_HOME; 
  case SCANCODE_END: 
    return KEY_END; 
    
  case SCANCODE_ESCAPE:
    return KEY_ESCAPE;
  case SCANCODE_1:
    return '1';
  case SCANCODE_2:
    return '2';
  case SCANCODE_3:
    return '3';
  case SCANCODE_4:
    return '4';
  case SCANCODE_5:
    return '5';
  case SCANCODE_6:
    return '6';
  case SCANCODE_7:
    return '7';
  case SCANCODE_8:
    return '8';
  case SCANCODE_9:
    return '9';
  case SCANCODE_0:
    return '0';
  case SCANCODE_MINUS:
    return '-';
  case SCANCODE_EQUAL:
    return '=';
  case SCANCODE_BACKSPACE:
    return KEY_BACKSPACE; 
  case SCANCODE_TAB:
    return KEY_TAB;
  case SCANCODE_Q:
    return 'q';
  case SCANCODE_W:
    return 'w';
  case SCANCODE_E:
    return 'e';
  case SCANCODE_R:
    return 'r';
  case SCANCODE_T:
    return 't';
  case SCANCODE_Y:
    return 'y';
  case SCANCODE_U:
    return 'u';
  case SCANCODE_I:
    return 'i';
  case SCANCODE_O:
    return 'o';
  case SCANCODE_P:
    return 'p';
  case SCANCODE_BRACKET_LEFT:
    return '[';
  case SCANCODE_BRACKET_RIGHT:
    return ']';
  case SCANCODE_ENTER:
    return KEY_ENTER;
  case SCANCODE_LEFTCONTROL:
    /* fall through */
  case SCANCODE_RIGHTCONTROL:
    return KEY_RCTRL;
  case SCANCODE_A:
    return 'a';
  case SCANCODE_S:
    return 's';
  case SCANCODE_D:
    return 'd';
  case SCANCODE_F:
    return 'f';
  case SCANCODE_G:
    return 'g';
  case SCANCODE_H:
    return 'h';
  case SCANCODE_J:
    return 'j';
  case SCANCODE_K:
    return 'k';
  case SCANCODE_L:
    return 'l';
  case SCANCODE_SEMICOLON:
    return ';';
  case SCANCODE_APOSTROPHE:
    return '\'';
  case SCANCODE_GRAVE:
    return '`';
  case SCANCODE_LEFTSHIFT:
    /* fall through */
  case SCANCODE_RIGHTSHIFT:
    return KEY_RSHIFT;
  case SCANCODE_BACKSLASH:
    return '\\';
  case SCANCODE_Z:
    return 'z';
  case SCANCODE_X:
    return 'x';
  case SCANCODE_C:
    return 'c';
  case SCANCODE_V:
    return 'v';
  case SCANCODE_B:
    return 'b';
  case SCANCODE_N:
    return 'n';
  case SCANCODE_M:
    return 'm';
  case SCANCODE_COMMA:
    return ',';
  case SCANCODE_PERIOD:
    return '.';
  case SCANCODE_SLASH:
    return '/';
  case SCANCODE_KEYPADMULTIPLY:
    return '*';
  case SCANCODE_LEFTALT:
    /* fall through */
  case SCANCODE_RIGHTALT:
    return KEY_RALT;
  case SCANCODE_SPACE:
    return ' ';
  case SCANCODE_F1:
    return KEY_F1;
  case SCANCODE_F2:
    return KEY_F2;
  case SCANCODE_F3:
    return KEY_F3;
  case SCANCODE_F4:
    return KEY_F4;
  case SCANCODE_F5:
    return KEY_F5;
  case SCANCODE_F6:
    return KEY_F6;
  case SCANCODE_F7:
    return KEY_F7;
  case SCANCODE_F8:
    return KEY_F8;
  case SCANCODE_F9:
    return KEY_F9;
  case SCANCODE_F10:
    return KEY_F10;
  case SCANCODE_F11:
    return KEY_F11;
  case SCANCODE_F12:
    return KEY_F12;
  case SCANCODE_KEYPADMINUS:
    return '-';
  case SCANCODE_KEYPADPLUS:
    return '=';
  case SCANCODE_KEYPADPERIOD:
    return '.';
  case SCANCODE_KEYPADENTER:
    return KEY_ENTER;
  case SCANCODE_KEYPADDIVIDE:
    return '/';
  case SCANCODE_BREAK:
    return KEY_PAUSE;
  case SCANCODE_BREAK_ALTERNATIVE:
    return KEY_PAUSE;
  case SCANCODE_CURSORBLOCKUP:
    return KEY_UPARROW;
  case SCANCODE_CURSORBLOCKLEFT:
    return KEY_LEFTARROW;
  case SCANCODE_CURSORBLOCKRIGHT:
    return KEY_RIGHTARROW;
  case SCANCODE_CURSORBLOCKDOWN:
    return KEY_DOWNARROW;
  default:
    /* Unhandled */
    break;
  }
  return 0;
}


/*
 * I_StartFrame
 */
void I_StartFrame (void)
{
  /* er? */
}

void I_GetEvent(void)
{
	event_t event;
	event_q current;

	while (1) {
		if (event_get(&current)) {
			switch (current.event) {
				case KEY_DOWN_EVENT:
					event.type = ev_keydown;
					event.data1 = keyxlat(current.data.key);
					D_PostEvent(&event);
					break;
				case KEY_UP_EVENT:
					event.type = ev_keyup;
					event.data1 = keyxlat(current.data.key);
					D_PostEvent(&event);
					break;
				case MOUSE_EVENT:
					event.type = ev_mouse;
					event.data1 = current.data.mouse.button;
					event.data2 = current.data.mouse.dx << 4;
					event.data3 = -current.data.mouse.dy << 4;
					D_PostEvent(&event);
					break;
			}
		} else {
			return;
		}
	}
}

/*
 * I_StartTic
 */
void I_StartTic (void)
{
	I_GetEvent();
}


/*
 * I_FinishUpdate
 */
void I_FinishUpdate (void)
{
	static int	lasttic;
	int		tics, i;

	/* draws little dots on the bottom of the screen */
	if (devparm) {
		i = I_GetTime();
		tics = i - lasttic;
		lasttic = i;
		if (tics > 20)
			tics = 20;
		for (i=0 ; i<tics*2 ; i+=2)
			screen[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
		for ( ; i<20*2 ; i+=2)
			screen[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x00;
	}

	/* Filters the first 32 palette-entry-color'd(gray-color) pixels if wanted */

	if (lifilter && !bilifilter)
    	   V_Filter_Screen_linear(screen);
  	if (bilifilter && !lifilter)
           V_Filter_Screen_bilinear(screen);	
	
	memcpy(vga_getgraphmem(), screen, SCREENWIDTH * SCREENHEIGHT);
}

/*
 * I_SetPalette
 */
void I_SetPalette (byte* palette)
{
	int i;
	struct {
		int	r;
		int	g;
		int	b;
	} col[256];

	for (i = 0; i < 256; i++) {
		col[i].r = gammatable[usegamma][*palette++] >> 2;
		col[i].g = gammatable[usegamma][*palette++] >> 2;
		col[i].b = gammatable[usegamma][*palette++] >> 2;
	}
	vga_setpalvec(0, 256, (int *) col);
}

void InitGraphLib(void)
{
  if (vga_init() != 0) {
	  I_Error("Not running in graphics capable console or could not find a free VC\n");
  }
}

struct {
	int type;
	char *typestr;
} mousetypes[] = {
	{ MOUSE_MICROSOFT, "Microsoft" },
	{ MOUSE_MOUSESYSTEMS, "Mousesystems" },
	{ MOUSE_MMSERIES, "MMSeries" },
	{ MOUSE_LOGITECH, "Logitech" },
	{ MOUSE_BUSMOUSE, "Busmouse" },
	{ MOUSE_PS2, "PS/2" },
	{ MOUSE_LOGIMAN, "LogiMan" },
#ifdef __NEWVGALIB__
	{ MOUSE_GPM, "GPM emulated" },
	{ MOUSE_SPACEBALL, "SpaceTec Spaceball" },
	{ MOUSE_INTELLIMOUSE, "Intellimouse" },
	{ MOUSE_IMPS2, "IMPS/2" },
	{ MOUSE_NONE, "None" },
#endif /* __NEWVGALIB__ */
	{ 0, NULL }
};

void do_from_background(void)
{
  lock_keys = 1;
}

void I_InitGraphics(void)
{
  static int		firsttime=1;
  int			mouse, i;
  struct sigaction	sact;
  
  if (!firsttime)
    return;
  firsttime = 0;
  
  sact.sa_handler = (void *)(int) I_Quit;
  sigfillset(&sact.sa_mask);
  sact.sa_flags = 0;
  sigaction(SIGINT, &sact, NULL);

  mouse = vga_getmousetype() & (~(MOUSE_CHG_DTR | MOUSE_DTR_HIGH
			  | MOUSE_CHG_RTS | MOUSE_RTS_HIGH));
  for (i = 0; mousetypes[i].typestr != NULL; i++) {
	  if (mouse == mousetypes[i].type)
		  break;
  }
  if (mousetypes[i].typestr == NULL) {
	  fprintf(stderr, "Unknown mouse type - not initialising.\n");
  } else {
	  if (mouse_init(mousedev, mouse, MOUSE_DEFAULTSAMPLERATE) == 0) {
		  mouse_seteventhandler(mouse_event_handler);
		  fprintf(stderr, "Mouse initialisation successful on %s type %s\n",
				  mousedev, mousetypes[i].typestr);
		  mouseworks = 1;
	  } else {
		  fprintf(stderr, "Mouse initialisation failed for %s type %s.\n",
				  mousedev, mousetypes[i].typestr);
	  }
  }
  if (!M_CheckParm("-nobgrun") && vga_runinbackground_version() == 1) {
        vga_runinbackground(VGA_COMEFROMBACK,do_from_background);
        vga_runinbackground(1);
  }

  if (vga_setmode(G320x200x256) != 0)
	  I_Error("Could not switch to mode 320 x 200 x 256\n");

  keyboard_init();
  keyboard_seteventhandler(kb_event_handler);
  memset(event_queue, 0, EVENT_BUFFER * sizeof(event_q));
  event_head = 0;
  event_tail = 0;

  screen = malloc(screenheight*screenwidth);
  assert(screen);
}

void I_ShutdownGraphics(void)
{
	if (mouseworks) {
		fprintf(stderr, "Shutting down mouse support\n");
		mouse_setdefaulteventhandler();
		mouse_close();
	}
	if (vga_getcurrentmode() != TEXT)
		vga_setmode(TEXT);
	keyboard_close();
}

void I_CheckRes()
{
  /* nothing to do... */
}

/* EOF */
