/*
 * include/kernel/i386/bits.h
 */

#ifndef _KERNEL_I386_BITS_H_
#define _KERNEL_I386_BITS_H_

static inline int bits_highest_set(int x)
{
 	int ret;
	asm volatile
	(
	 "bsrl %1, %0"
	 : "=r" (ret)
	 : "r" (x)
	);
	return ret;
}

static inline int bits_lowest_set(int x)
{
	int ret;
	asm volatile
	(
	 "bsfl %1, %0"
	: "=r" (ret)
	: "r" (x)
	);
	return ret;
}

#endif
