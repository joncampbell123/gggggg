
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

/*=================================game loop===========================*/
void game_main(void) {
}

/*=================================program entry point=================================*/
int main(int argc,char **argv,char **envp) {
    probe_dos();
    cpu_probe();
    detect_windows();
    probe_8237();
    if (!probe_8259()) {
        printf("Cannot init 8259 PIC\n");
        return 1;
    }
    if (!probe_8254()) {
        printf("Cannot init 8254 timer\n");
        return 1;
    }
	probe_vga2();
    if ((vga2_flags & (VGA2_FLAG_CGA|VGA2_FLAG_EGA|VGA2_FLAG_VGA|VGA2_FLAG_MCGA)) == 0) {
        printf("This game requires support for CGA graphics\n");
        return 1;
    }

    /* timer setup */
	p8259_mask(T8254_IRQ);
    tick_calldown = 0;
    tick_calldown_add = (unsigned int)(T8254_REF_CLOCK_HZ / (unsigned long)TIMER_TICK_RATE);
    write_8254_system_timer((t8254_time_t)tick_calldown_add);
    old_tick_irq = _dos_getvect(irq2int(0));
    _dos_setvect(irq2int(0),tick_timer_irq);
    p8259_unmask(T8254_IRQ);

    /* setup graphics */
    set_int10_mode(4); /* CGA 320x200 4-color */

    /* game main */
    game_main();

    /* unsetup graphics */
    set_int10_mode(3);

    /* timer unsetup */
	timer_flush_events();
	callback_flush_events();
	p8259_mask(T8254_IRQ);
    write_8254_system_timer(0x10000ul); /* restore normal timer */
    _dos_setvect(irq2int(0),old_tick_irq);
	p8259_unmask(T8254_IRQ);
    return 0;
}

