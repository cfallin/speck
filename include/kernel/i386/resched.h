/*
 * include/kernel/i386/resched.h
 */

#ifndef _KERNEL_I386_RESCHED_H_
#define _KERNEL_I386_RESCHED_H_

#define resched() \
	asm volatile("int $0x81")

#endif
