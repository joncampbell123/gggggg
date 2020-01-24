
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

unsigned int            TIMER_TICK_RATE = 300;

volatile uint32_t       tick_count = 0;
void                    (__interrupt __far *old_tick_irq)() = NULL;

void __interrupt __far tick_timer_irq() {
    tick_count++;
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
    write_8254_system_timer((t8254_time_t)(T8254_REF_CLOCK_HZ / (unsigned long)TIMER_TICK_RATE));
    old_tick_irq = _dos_getvect(irq2int(0));
    _dos_setvect(irq2int(0),tick_timer_irq);
    p8259_unmask(T8254_IRQ);

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

