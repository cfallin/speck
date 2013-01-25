/*
 * include/kernel/mutex.h
 */

#ifndef _KERNEL_MUTEX_H_
#define _KERNEL_MUTEX_H_

#include <kernel/sem.h>

typedef struct mutex
{
	sem s;
	int owner;
} mutex;

int mutex_lock(mutex *m);
int mutex_unlock(mutex *m);

int mutex_init(mutex *m);
int mutex_free(mutex *m);

#endif
