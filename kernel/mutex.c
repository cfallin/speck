/*
 * kernel/mutex.c
 */

#include <kernel/mutex.h>
#include <kernel/sem.h>
#include <kernel/process.h>

int mutex_lock(mutex *m)
{
	if(process_current[smp_cpu_id()] && process_current[smp_cpu_id()]->pid
		== m->owner)
		return 0;

	sem_down(&m->s);
	m->owner = process_current[smp_cpu_id()] ? 
		process_current[smp_cpu_id()]->pid :
		0;

	return 0;
}

int mutex_unlock(mutex *m)
{
	if(process_current[smp_cpu_id()] && process_current[smp_cpu_id()]->pid !=
		m->owner)
		return -1;

	m->owner = -1;
	sem_up(&m->s);

	return 0;
}

int mutex_init(mutex *m)
{
	sem_init(&m->s, 1);
	m->owner = -1;

	return 0;
}

int mutex_free(mutex *m)
{
	sem_free(&m->s);

	return 0;
}
