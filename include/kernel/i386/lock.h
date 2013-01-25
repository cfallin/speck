/*
 * include/kernel/i386/lock.h
 */

#ifndef _KERNEL_I386_LOCK_H_
#define _KERNEL_I386_LOCK_H_

int atomic_add(int *ptr, int i);
int atomic_sub(int *ptr, int i);
int atomic_test_and_set(int *ptr);
int atomic_xchg(int *ptr, int newval);

void spinlock_grab(int *lock);
void spinlock_release(int *lock);

#endif
