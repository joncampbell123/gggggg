
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <hw/cpu/endian.h>
#include <conio.h> /* this is where Open Watcom hides the outp() etc. functions */
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <malloc.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <dos.h>

#include <hw/dos/dos.h>
#include <hw/cpu/cpu.h>
#include <hw/vga2/vga2.h>
#include <hw/8237/8237.h>       /* 8237 DMA */
#include <hw/8254/8254.h>       /* 8254 timer */
#include <hw/8259/8259.h>       /* 8259 PIC interrupts */
#include <hw/sndsb/sndsb.h>
#include <hw/vga/vgatty.h>
#include <hw/dos/doswin.h>

/*============================= util ===========================*/
void set_int10_mode(const unsigned int c);
#pragma aux set_int10_mode = \
    "int 10h" \
    parm [ax] \
    modify [ax];

#define VIDEO_INIT_MODE         (1u << 0u)

unsigned char                   video_init_state = 0;
unsigned int                    video_height = 0;
unsigned int                    video_width = 0;
void                            (*video_sysmsgbox)(const char *title,const char *msg) = NULL;

/* Open Watcom allows __segment types and void based variables to represent far ptrs, so why not use it
 * to avoid constant reloading segment registers involved with unsigned char far *
 * This works because CGA video memory is 16KB total, in a 32KB region. */
static const __segment          vmseg = 0xB800u;

unsigned char far*              video_font_8x8_p1 = NULL;
unsigned char far*              video_font_8x8_p2 = NULL;

unsigned int strnewlinecount(const char *s) {
    unsigned int c = 0;

    while (*s != 0) {
        if (*s == '\n') c++;
        s++;
    }

    return c;
}

static inline unsigned int video_ptrofs(unsigned int x,unsigned int y) {
    return ((y >> 1u) * 80) + (x >> 2u) + ((y & 1u) << 13u);
}

static inline unsigned int video_scanlineadv(const unsigned int p) {
    if (p & 0x2000u)
        return (p + 80ul - 0x2000ul);
    else
        return (p + 0x2000ul);
}

static inline unsigned char cga4dup(const unsigned int color) {
    const unsigned char s1 = color + (color << 4u);
    return s1 + (s1 << 2u);
}

static inline unsigned char cga4leftmask(const unsigned int x) {
    const unsigned char shf = (x & 3u) << 1u;
    return 0xFFu >> shf;
}

static inline unsigned char cga4rightmask(const unsigned int x) {
    const unsigned char shf = ((~x) & 3u) << 1u;
    return 0xFF << shf;
}

static inline unsigned char far *video_vp2ptr(const unsigned int vp) {
    return (unsigned char far*)(vmseg:>((unsigned char __based(void) *)(vp)));
}

static inline void video_wrvmaskv(const unsigned int vp,const unsigned char mask,const unsigned char v) {
    unsigned char far * const p = video_vp2ptr(vp);
    *p = (*p & (~mask)) + (v & mask);
}

static inline void video_wrmaskv(const unsigned int vp,const unsigned char mask,const unsigned char v) {
    unsigned char far * const p = video_vp2ptr(vp);
    *p = (*p & mask) + v;
}

static inline void video_wr(const unsigned int vp,const unsigned char v) {
    *video_vp2ptr(vp) = v;
}

void video_hline(unsigned int x1,unsigned int x2,unsigned int y,unsigned int color) {
    if (x1 <= x2) {
        const unsigned char wbm = cga4dup(color);
        unsigned int vp = video_ptrofs(x1,y);
        unsigned char x1b = x1 >> 2u;
        unsigned char x2b = x2 >> 2u;

        if (x1b != x2b) {
            if (x1 & 3u) {
                const unsigned char mask = cga4leftmask(x1);
                video_wrvmaskv(vp++,mask,wbm);
                x1b++;
            }
            while (x1b < x2b) {
                video_wr(vp++,wbm);
                x1b++;
            }
            /* assume x1b == x2b */
            video_wrvmaskv(vp++,cga4rightmask(x2),wbm);
        }
        else {
            video_wrvmaskv(vp++,cga4leftmask(x1) & cga4rightmask(x2),wbm);
        }
    }
}

void video_vline(unsigned int y1,unsigned int y2,unsigned int x,unsigned int color) {
    const unsigned char shf = ((~x) & 3u) << 1u;
    const unsigned char mask = ~(3u << shf);
    const unsigned char wb = color << shf;
    unsigned int vp = video_ptrofs(x,y1);

    while (y1 <= y2) {
        video_wrmaskv(vp,mask,wb);
        vp = video_scanlineadv(vp);
        y1++;
    }
}

void video_solidbox(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2,unsigned int color) {
    for (;y1 <= y2;y1++)
        video_hline(x1,x2,y1,color);
}

void video_rectbox(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2,unsigned int color) {
    if (y1 < y2) {
        video_hline(x1,x2,y1,color);
        video_hline(x1,x2,y2,color);
        if (x1 < x2) {
            video_vline(y1+1u,y2-1u,x1,color);
            video_vline(y1+1u,y2-1u,x2,color);
        }
    }
    else if (y1 == y2) {
        video_hline(x1,x2,y1,color);
    }
}

static inline unsigned char video_mask_fb4_u12(const uint16_t bitm) {
    unsigned char mask = 0;

    if (bitm & 0x8000u) mask |= 0xC0;
    if (bitm & 0x4000u) mask |= 0x30;
    if (bitm & 0x2000u) mask |= 0x0C;
    if (bitm & 0x1000u) mask |= 0x03;

    return mask;
}

void video_print8x8(unsigned int x,unsigned int y,unsigned char color,unsigned char far *fbmp) {
    const unsigned char wbm = cga4dup(color);
    unsigned int vp = video_ptrofs(x,y);
    unsigned char bitmshf = 8u - (x & 3u);
    unsigned char h = 8;
    uint16_t bitm;

    do {
        bitm = ((uint16_t)(*fbmp++)) << bitmshf;

        { /* will draw at minimum 8 pixels, maximum 12 */
            unsigned int vp2 = vp;
            video_wrvmaskv(vp2++,video_mask_fb4_u12(bitm),wbm); bitm <<= 4u;
            video_wrvmaskv(vp2++,video_mask_fb4_u12(bitm),wbm); bitm <<= 4u;
            if (bitm != 0u) video_wrvmaskv(vp2++,video_mask_fb4_u12(bitm),wbm);
        }

        vp = video_scanlineadv(vp);
    } while (--h != 0u);
}

unsigned char far *video8x8fontlookup(const unsigned char c) {
    if (c & 0x80u)
        return video_font_8x8_p2 + (((unsigned int)((unsigned char)(c - 0x80u))) * 8u);
    else
        return video_font_8x8_p1 + (((unsigned int)((unsigned char)c)) * 8u);
}

void video_sysmsgbox_cga4(const char *title,const char *msg) { /* assume 320x200 */
    const unsigned int box_left = 0u;
    const unsigned int box_right = video_width - 1u;
    unsigned int lines = 1u/*title*/ + 1u/*space*/ + 1u/*msg*/ + strnewlinecount(msg)/*additional lines*/;
    unsigned int y = (video_height - ((lines * 8u) + 2u)) / 2; /* center */
    unsigned int lmargin = 1u;
    unsigned int x;
    char c;

    video_rectbox(box_left,y,box_right,y+(lines*8u)+2u-1u,1/*red/magenta*/); /* coords inclusive */
    video_solidbox(box_left+1u,y+1u,box_right-1u,y+(lines*8u)+2u-2u,3/*yellow/white*/); /* coords inclusive */

    /* step past border */
    y++;

    /* title */
    x = lmargin;
    while ((c = (*title++)) != 0) {
        video_print8x8(x,y,2u,video8x8fontlookup(c));
        x += 8u;
    }

    /* title + gap */
    y += 8u * 2u;

    /* message */
    x = lmargin;
    while ((c = (*msg++)) != 0) {
        if (c == '\n') {
            x = lmargin;
            y += 8u;
        }
        else {
            video_print8x8(x,y,0u,video8x8fontlookup(c));
            x += 8u;
        }
    }
}

/*============================= timer/event =================================*/
#define                                 TIMER_EVENT_FLAG_IRQ                    (1u << 0u)

struct timer_event_t {
    const char*                         name;
    unsigned int                        flags;
    unsigned int                        time;
    uint32_t                            user;
    void                                (*callback)(uint32_t user);
    volatile struct timer_event_t*      next;
};

#define                         TIMER_IRQ_COUNT_RESET_AT (10000U)
#define                         TIMER_IRQ_COUNT_RESET_SUB (5000U)
#define                         TIMER_TICK_RATE (300U)

volatile uint32_t               tick_count = 0;     /* tick counter for use with game engine in general */
volatile unsigned int           tick_irq_count = 0; /* tick counter used for scheduling events, which may reset from time to time */
unsigned int                    tick_calldown = 0;
unsigned int                    tick_calldown_add = 0;
void                            (__interrupt __far *old_tick_irq)() = NULL;

volatile struct timer_event_t*  timer_next = NULL;
volatile struct timer_event_t*  timer_fired_nonirq = NULL;

static inline void add_fired_nonirq(volatile struct timer_event_t *ev) {
    ev->next = timer_fired_nonirq;
    timer_fired_nonirq = ev;
}

void timer_count_reset(void) {
    SAVE_CPUFLAGS( _cli() ) {
        tick_count = 0;
    } RESTORE_CPUFLAGS();
}

void timer_flush_events(void) {
    volatile struct timer_event_t* p;

    SAVE_CPUFLAGS( _cli() ) {
        p = timer_next;
        timer_next = NULL;
    } RESTORE_CPUFLAGS();

    while (p != NULL) {
        volatile struct timer_event_t* n = p->next;
        p->next = NULL;
        p = n;
    }
}

void callback_flush_events(void) {
    volatile struct timer_event_t* p;

    SAVE_CPUFLAGS( _cli() ) {
        p = timer_fired_nonirq;
        timer_fired_nonirq = NULL;
    } RESTORE_CPUFLAGS();

    while (p != NULL) {
        volatile struct timer_event_t* n = p->next;
        p->next = NULL;
        p = n;
    }
}

void callback_nonirq_event(void) {
    volatile struct timer_event_t* p;

    do {
        SAVE_CPUFLAGS( _cli() ) {
            p = timer_fired_nonirq;
            if (p != NULL) timer_fired_nonirq = p->next;
        } RESTORE_CPUFLAGS();

        if (p != NULL)
            p->callback(p->user);
        else
            break;
    } while (1);
}

void __interrupt __far tick_timer_irq() {
    while (timer_next != NULL && tick_irq_count >= timer_next->time) {
        volatile struct timer_event_t *current = timer_next;
        volatile struct timer_event_t *next = current->next;

        timer_next = next;
        current->next = NULL;

        if (current->flags & TIMER_EVENT_FLAG_IRQ)
            current->callback(current->user);
        else
            add_fired_nonirq(current);
    }

    if (tick_irq_count >= TIMER_IRQ_COUNT_RESET_AT) {
        volatile struct timer_event_t *e = timer_next;

        tick_irq_count -= TIMER_IRQ_COUNT_RESET_SUB;
        for (;e != NULL;e=e->next) e->time -= TIMER_IRQ_COUNT_RESET_SUB;
    }

    tick_irq_count++;
    tick_count++;

    {
        const uint32_t ncnt = (uint32_t)tick_calldown + (uint32_t)tick_calldown_add;
        if (ncnt == (uint32_t)0/*add==0*/ || (ncnt & 0x10000ul)/*overflow*/)
            old_tick_irq();
        else
            p8259_OCW2(0,P8259_OCW2_NON_SPECIFIC_EOI);

        tick_calldown = (uint16_t)ncnt;
    }
}

void remove_timer_event(volatile struct timer_event_t *ev) {
    if (ev != NULL) {
        SAVE_CPUFLAGS( _cli() ) {
            if (timer_next == ev) {
                timer_next = timer_next->next;
                ev->next = NULL;
            }
            else {
                volatile struct timer_event_t *s = timer_next;

                while (s != NULL) {
                    if (s->next == ev) {
                        s->next = s->next->next;
                        ev->next = NULL;
                        break;
                    }

                    s = s->next;
                }
            }
        } RESTORE_CPUFLAGS();
    }
}

void schedule_timer_event(volatile struct timer_event_t *ev,uint32_t time) {
    if (ev->next != NULL)
        return; /* ev->next if already scheduled */

    SAVE_CPUFLAGS( _cli() ) {
        ev->time = time + tick_irq_count;

        if (timer_next == NULL) {
            /* list is empty, add event to top */
            ev->next = NULL;
            timer_next = ev;
        }
        else if (ev->time < timer_next->time) {
            /* new event is newer than top of list, add to top */
            ev->next = timer_next;
            timer_next = ev;
        }
        else {
            volatile struct timer_event_t *s = timer_next;

            while (s != NULL) {
                if (s->next != NULL) {
                    /* if the next entry happens later, insert here */
                    if (ev->time < s->next->time) {
                        ev->next = s->next; /* old s->next */
                        s->next = ev; /* new s->next */
                        break;
                    }
                    /* keep looking */
                    else {
                        s = s->next;
                    }
                }
                else {
                    /* stick it on the end */
                    ev->next = NULL; /* s->next == NULL */
                    s->next = ev;
                    break;
                }
            }
        }
    } RESTORE_CPUFLAGS();
}

void timer_setup(void) {
    if (old_tick_irq == NULL) {
        /* timer setup */
        p8259_mask(T8254_IRQ);
        tick_calldown = 0;
        tick_calldown_add = (unsigned int)(T8254_REF_CLOCK_HZ / (unsigned long)TIMER_TICK_RATE);
        write_8254_system_timer((t8254_time_t)tick_calldown_add);
        old_tick_irq = _dos_getvect(irq2int(0)); /* NTS: It's unlikely IRQ0 is NULL, or else the system is one timer tick away from crashing! */
        _dos_setvect(irq2int(0),tick_timer_irq);
        p8259_unmask(T8254_IRQ);
    }
}

void timer_shutdown(void) {
    if (old_tick_irq != NULL) {
        /* timer unsetup */
        timer_flush_events();
        callback_flush_events();
        p8259_mask(T8254_IRQ);
        write_8254_system_timer(0x10000ul); /* restore normal timer */
        _dos_setvect(irq2int(0),old_tick_irq);
        p8259_unmask(T8254_IRQ);
    }
}

/*=================================game loop===========================*/
#define GAME_FLAG_STARTUP   (1u << 0u)
#define GAME_FLAG_INIT      (1u << 1u)
#define GAME_FLAG_SHUTDOWN  (1u << 2u)

unsigned char               game_flags = 0;

void game_main(void) {
}

void game_error(const char *msg) {
    if (video_init_state & VIDEO_INIT_MODE) {
        (*video_sysmsgbox)("ERROR",msg);
    }
    else {
        printf("ERROR: %s\n",msg);
    }
}

int video_setup(void) {
    if (!(video_init_state & VIDEO_INIT_MODE)) {
        if ((vga2_flags & (VGA2_FLAG_CGA|VGA2_FLAG_EGA|VGA2_FLAG_VGA|VGA2_FLAG_MCGA)) == 0) {
            game_error("This game requires support for CGA graphics");
            return -1;
        }

        /* setup graphics */
        /* TODO: Does INT 10h AH=0Fh (get current video mode) exist on PC/XT systems from 1984? */
        set_int10_mode(4); /* CGA 320x200 4-color */

        video_width = 320;
        video_height = 200;
        video_sysmsgbox = video_sysmsgbox_cga4;
        video_font_8x8_p1 = MK_FP(0xF000,0xFA6E);
        video_font_8x8_p2 = (unsigned char far*)_dos_getvect(0x1F);

        video_init_state |= VIDEO_INIT_MODE;
    }

    return 0;
}

int game_init(void) {
    /* only if the game has not been initialized, is not already starting up, and is not shutting down */
    if ((game_flags & (GAME_FLAG_STARTUP|GAME_FLAG_INIT|GAME_FLAG_SHUTDOWN)) == 0) {
        game_flags |= GAME_FLAG_STARTUP;

        probe_dos();
        cpu_probe();
        detect_windows();
        probe_8237(); // DMA
        probe_vga2();

        if (!probe_8259()) {
            game_error("Cannot init 8259 PIC");
            goto fail;
        }
        if (!probe_8254()) {
            game_error("Cannot init 8254 timer");
            goto fail;
        }

        timer_setup();
        if (video_setup() < 0) {
            game_error("Unable to initialize video");
            goto fail;
        }

        game_flags &= ~GAME_FLAG_STARTUP;
        game_flags |=  GAME_FLAG_INIT;
    }

    return 0;
fail:
    game_flags &= ~(GAME_FLAG_STARTUP|GAME_FLAG_INIT);
    return -1;
}

void video_shutdown(void) {
    if (video_init_state & VIDEO_INIT_MODE) {
        set_int10_mode(3);
        video_init_state &= ~VIDEO_INIT_MODE;
    }
}

void game_shutdown(void) {
    /* only if the game is not already shutting down and is either starting up or inited */
    if ((game_flags & (GAME_FLAG_SHUTDOWN)) == 0 &&
        (game_flags & (GAME_FLAG_INIT|GAME_FLAG_STARTUP)) != 0) {
        game_flags |=  GAME_FLAG_SHUTDOWN;
        game_flags &= ~GAME_FLAG_INIT;

        timer_shutdown();
        video_shutdown();

        game_flags &= ~(GAME_FLAG_STARTUP|GAME_FLAG_SHUTDOWN);
    }
}

/*=================================program entry point=================================*/
int main(int argc,char **argv,char **envp) {
    if (game_init() < 0) {
        game_shutdown();
        return 1;
    }

    game_error("Hello world");
    getch();
    game_error("Hello, yes?");
    getch();
    game_error("Hello world there\nHi hi\nNew lines");
    getch();
    game_error("Blah blah blah blah blah blah\nblah blah blah yadda daadadadad\niokwqroiuwqieouwq\nweuiryqwoeuiyqwuiey\nwqiueyqwewqeqwuieyqwui\nqwiueqwuieywqiueyw\nqweiuywqeiuyqwuieyqwuie\nqwiueryqwe\nuiyqweuiyqwuie\nuiyqweuiqwe\nioqwueiouqwe\niowqueiouqweioewq\nwqeroiqweiowq\nwerwqe");
    getch();

    /* game main */
    game_main();

    game_shutdown();
    return 0;
}

