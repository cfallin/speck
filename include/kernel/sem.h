/*
 * include/kernel/sem.h
 */

#ifndef _KERNEL_SEM_H_
#define _KERNEL_SEM_H_

#include <kernel/process.h>

typedef struct sem
{
	process_queue queue;
	int lock;
	int count;
} sem;

void sem_init(sem *s, int count);
void sem_free(sem *s);

void sem_up(sem *s);
void sem_down(sem *s);

#endif
