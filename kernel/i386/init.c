/*
 * kernel/i386/init.c
 */

#include <kernel/i386/init.h>
#include <kernel/i386/int.h>
#include <kernel/i386/vid.h>
#include <kernel/i386/io.h>

// for lack of a better place...
void timer_init()
{
	// repgrogram the timer (IRQ 0) for 100 Hz
	outb(0x36,0x43);
	outb( (11392 & 0xff)  ,0x40);
	outb(((11392>>8)&0xff),0x40);
}

void arch_init()
{
	int_init();
	timer_init();
	vid_init();

	vid_clear();

	vid_puts("speck/i386 initializing\n");
}
