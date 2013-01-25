/*
 * kernel/i386/panic.c
 */

#include <kernel/i386/panic.h>
#include <kernel/i386/vid.h>

// a note about the use of this function:
// we really shouldn't be using video-driver-internal functions, but panics
// can occur because of locking badness, and the standard video driver uses
// locking (levels and spinlocks) - this can cause infinite loops/deadlocks
// or other undesirable side effects, so the choice is really between this
// or possible video buffer corruption (but it won't matter anyway when the
// machine crashes)
void vid_puts_internal(char *s);

void panic(char *str)
{
	vid_puts_internal("\n\nPANIC: ");
	vid_puts_internal(str);
	asm volatile("cli;hlt");
}
