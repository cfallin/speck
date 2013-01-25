/*
 * kernel/i386/lock.c
 */

#include <kernel/i386/lock.h>
#include <kernel/i386/smp.h>

// for atomicity, we only need to lock the bus on SMP systems
#if SMP_MAX_CPUS > 1
#define LOCK "lock;"
#else
#define LOCK ""
#endif

int atomic_add(int *ptr, int i)
{
	int r;
	asm volatile
	(
	 LOCK "xaddl %%eax, (%%ebx)"
	 :"=a" (r)
	 :"0" (i), "b" ((int)ptr)
	);
	return r;
}

int atomic_sub(int *ptr, int i)
{
	int r;
	asm volatile
	(
	 LOCK "xaddl %%eax, (%%ebx)"
	 :"=a" (r)
	 :"0" (-i), "b" ((int)ptr)
	);
	return r;
}

int atomic_test_and_set(int *ptr)
{
	int r;
	asm volatile
	(
	 LOCK "bts $0, (%1)\n"
	 "xorl %%eax, %%eax\n"
	 "setc %%al\n"
	 : "=a" (r)
	 : "r" ((int)ptr)
	);
	return r;
}

int atomic_xchg(int *ptr, int newval)
{
	int oldval;
	asm volatile
	(
	 "xchgl (%1), %0"
	 : "=r" (oldval)
	 : "0" (newval), "r" ((int)ptr)
	 : "memory"
	);
	return oldval;
}

#if SMP_MAX_CPUS > 1

void spinlock_grab(int *lock)
{
	while(atomic_test_and_set(lock));
}

void spinlock_release(int *lock)
{
	*lock = 0;
}

#else

void spinlock_grab(int *lock) {}
void spinlock_release(int *lock) {}

#endif
