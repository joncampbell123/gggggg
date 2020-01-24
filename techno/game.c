
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

#define TIMER_EVENT_FLAG_IRQ            (1u << 0u)

struct timer_event_t {
    unsigned int                        flags;
    uint32_t                            time;
    uint32_t                            user;
    void                                (*callback)(uint32_t user);
    volatile struct timer_event_t*      next;
};

#define                         TIMER_IRQ_COUNT_RESET_AT (100000UL)
#define                         TIMER_IRQ_COUNT_RESET_SUB (50000UL)
#define                         TIMER_TICK_RATE (300UL)

volatile uint32_t               tick_count = 0;
volatile uint32_t               tick_irq_count = 0;
void                            (__interrupt __far *old_tick_irq)() = NULL;

volatile struct timer_event_t*  timer_next_irq = NULL;
volatile struct timer_event_t*  timer_fired_nonirq = NULL;

static inline void add_fired_nonirq(volatile struct timer_event_t *ev) {
    ev->next = timer_fired_nonirq;
    timer_fired_nonirq = ev;
}

void __interrupt __far tick_timer_irq() {
    while (timer_next_irq != NULL && tick_irq_count >= timer_next_irq->time) {
        volatile struct timer_event_t *current = timer_next_irq;
        volatile struct timer_event_t *next = current->next;

        timer_next_irq = next;
        current->next = NULL;

        if (current->flags & TIMER_EVENT_FLAG_IRQ)
            current->callback(current->user);
        else
            add_fired_nonirq(current);
    }

    if (tick_irq_count >= TIMER_IRQ_COUNT_RESET_AT) {
        volatile struct timer_event_t *e = timer_next_irq;

        tick_irq_count -= TIMER_IRQ_COUNT_RESET_SUB;
        for (;e != NULL;e=e->next) e->time -= TIMER_IRQ_COUNT_RESET_SUB;
    }

    tick_irq_count++;
    tick_count++;

    p8259_OCW2(0,P8259_OCW2_NON_SPECIFIC_EOI);
}

void schedule_timer_irq_event(volatile struct timer_event_t *ev,uint32_t time) {
    if (ev->next != NULL)
        return; /* ev->next if already scheduled */

    SAVE_CPUFLAGS( _cli() ) {
        ev->time = time + tick_irq_count;

        if (timer_next_irq == NULL) {
            /* list is empty */
            ev->next = NULL;
            timer_next_irq = ev;
        }
        else if (ev->time < timer_next_irq->time) {
            /* new event is newer than top of list */
            ev->next = timer_next_irq;
            timer_next_irq = ev;
        }
        else {
            volatile struct timer_event_t *s = timer_next_irq;

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

void beeper_cb(uint32_t t);
void blah2_cb(uint32_t t);
void blah_cb(uint32_t t);

struct timer_event_t blah = {
    TIMER_EVENT_FLAG_IRQ,//flags
    0,//delay
    0,//user
    blah_cb,//callback
    NULL//next
};

struct timer_event_t blah2 = {
    TIMER_EVENT_FLAG_IRQ,//flags
    0,//delay
    0,//user
    blah2_cb,//callback
    NULL//next
};

struct timer_event_t beeper = {
    TIMER_EVENT_FLAG_IRQ,//flags
    0,//delay
    0,//user
    beeper_cb,//callback
    NULL//next
};

void blah_cb(uint32_t t) {
    (void)t;

    ( *((uint16_t far*)MK_FP(0xB800,0x0000)) )++;

    schedule_timer_irq_event(&blah,100);
}

void blah2_cb(uint32_t t) {
    (void)t;

    ( *((uint16_t far*)MK_FP(0xB800,0x0002)) )++;

    schedule_timer_irq_event(&blah2,3);
}

void beeper_cb(uint32_t t) {
    (void)t;

    __asm {
        push    ax
        in      al,61h
        xor     al,3h
        out     61h,al
        pop     ax
    }

    schedule_timer_irq_event(&beeper,300);
}

int main(int argc,char **argv,char **envp) {
#if 1
    uint32_t ptick_count = 0;
#endif

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
    if ((vga2_flags & (VGA2_FLAG_CGA|VGA2_FLAG_EGA|VGA2_FLAG_VGA)) == 0) {
        printf("This game requires support for CGA graphics\n");
        return 1;
    }

    /* timer setup */
	p8259_mask(T8254_IRQ);
    write_8254_system_timer((t8254_time_t)(T8254_REF_CLOCK_HZ / (unsigned long)TIMER_TICK_RATE));
    old_tick_irq = _dos_getvect(irq2int(0));
    _dos_setvect(irq2int(0),tick_timer_irq);
    p8259_unmask(T8254_IRQ);

    schedule_timer_irq_event(&beeper,300);
    schedule_timer_irq_event(&blah,100);
    schedule_timer_irq_event(&blah2,3);

    for (;;) {
        if (kbhit()) {
            int c = getch();
            if (c == 27) break;

            if (c == ' ') {
                p8259_mask(T8254_IRQ);
                {
                    volatile struct timer_event_t *t = timer_next_irq;
                    fprintf(stderr,"\n");
                    while (t != NULL) {
                        fprintf(stderr,"t=%lu p=%p\n",(unsigned long)t->time,(void*)t);
                        t = t->next;
                    }
                    fprintf(stderr,"\n");
                }
                p8259_unmask(T8254_IRQ);
            }
        }

#if 1
        {
            uint32_t t,ti;
            SAVE_CPUFLAGS( _cli() ) {
                t = tick_count;
                ti = tick_irq_count;
            } RESTORE_CPUFLAGS();
            if (ptick_count != t) {
                ptick_count = t;
                printf("\x0D" "%lu/IRQ=%lu  ",(unsigned long)t,(unsigned long)ti);
                fflush(stdout);
            }
        }
#endif
    }

    /* timer unsetup */
	p8259_mask(T8254_IRQ);
    write_8254_system_timer(0x10000ul); /* restore normal timer */
    _dos_setvect(irq2int(0),old_tick_irq);
	p8259_unmask(T8254_IRQ);
    return 0;
}

