/*
 * kernel/sem.c
 */

#include <kernel/sem.h>
#include <kernel/level.h>
#include <kernel/process.h>
#include <kernel/queue.h>
#include <kernel/malloc.h>
#include <kernel/arch/lock.h>
#include <kernel/arch/string.h>
#include <kernel/arch/panic.h>
#include <kernel/arch/resched.h>

void sem_init(sem *s, int count)
{
	// initialize the struct
	memset(s,0,sizeof(sem));
	s->count = count;
}

void sem_free(sem *s)
{
	if(s->queue.head)
		panic("Semaphore with waiting process is being freed");

	// nothing else here - responsibility of user to free their struct sem
}

void sem_up(sem *s)
{
	int level;
	int i;

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&s->lock);

	// inc the count - will be above 0 if there are avail resources
	s->count++;

	// if we can allow waiting processes to run, do so
	i = s->count;
	while((i-- > 0) && s->queue.head)
	{
		process *p;

		p = s->queue.head;

		queue_remove(p);
		queue_insert(&process_queues[p->prio],p);

		process_queues_runnable_mask |= (1 << p->prio);
	}

	spinlock_release(&s->lock);
	level_return(level);
}

void sem_down(sem *s)
{
	int level;

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&s->lock);

	// keep trying until there are avail resources
	while(s->count <= 0)
	{
		process *p = process_current[smp_cpu_id()];

		// if no avail resources then sleep on the queue
		queue_insert(&s->queue,p);

		spinlock_release(&s->lock);
		level_return(level);

		resched();

		level = level_go(LEVEL_NOINTS);
		spinlock_grab(&s->lock);
	}

	s->count--;

	spinlock_release(&s->lock);
	level_return(level);
}
