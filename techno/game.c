
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

static inline void write_8254_system_timer_timeout(t8254_time_t max) {
    write_8254(T8254_TIMER_INTERRUPT_TICK,max,T8254_MODE_0_INT_ON_TERMINAL_COUNT);
}

volatile uint32_t       tick_count = 0;
t8254_time_t            tick_schedule_delay = 0;
void                    (__interrupt __far *old_tick_irq)() = NULL;

void schedule_tick(unsigned long count) {
	p8259_mask(T8254_IRQ);

    if (count > tick_schedule_delay) {
        write_8254_system_timer_timeout((t8254_time_t)count - tick_schedule_delay);
        p8259_unmask(T8254_IRQ);
    }
}

void __interrupt __far tick_timer_irq() {
    tick_count++;

    schedule_tick(t8254_us2ticks(10000ul));

    p8259_OCW2(0,P8259_OCW2_NON_SPECIFIC_EOI);
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

    /* setup delay detect */
    {
        SAVE_CPUFLAGS( _cli() ) {
            write_8254_system_timer_timeout(0ul);
            tick_schedule_delay = (t8254_time_t)(0x10000ul) - read_8254_ncli(T8254_TIMER_INTERRUPT_TICK);
        } RESTORE_CPUFLAGS();
#if 1
        printf("8254 schedule delay: %lu ticks\n",(unsigned long)tick_schedule_delay);
#endif
    }

    /* start tick event handling */
    old_tick_irq = _dos_getvect(irq2int(0));
    _dos_setvect(irq2int(0),tick_timer_irq);
    write_8254_system_timer_timeout(1ul);
    schedule_tick(t8254_us2ticks(10000ul));

    for (;;) {
        if (kbhit()) {
            int c = getch();
            if (c == 27) break;
        }

#if 1
        {
            uint32_t t;
            SAVE_CPUFLAGS( _cli() ) {
                t = tick_count;
            } RESTORE_CPUFLAGS();
            if (ptick_count != t) {
                ptick_count = t;
                printf("\x0D" "%lu  ",(unsigned long)ptick_count);
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

