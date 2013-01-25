/*
 * kernel/irqmsg.c
 */

#include <kernel/irqmsg.h>
#include <kernel/process.h>
#include <kernel/arch/lock.h>
#include <kernel/level.h>
#include <kernel/queue.h>
#include <kernel/arch/resched.h>
#include <kernel/arch/int.h>
#include <kernel/arch/bits.h>

process_queue irqmsg_queue;
int irqmsg_queue_spinlock;

int irqmsg_handler_count[INT_MAX_IRQS];

int irqmsg_register(int mask)
{
	process *self;
	int level;
	int oldmask,maskdiff;
	int i;

	self = process_current[smp_cpu_id()];

	// save the old mask
	oldmask = self->irqmsg_mask;

	// set our IRQ mask
	self->irqmsg_mask = mask;

	// put ourselves on the irq message queue
	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&irqmsg_queue_spinlock);

	if(mask && !oldmask)
	{
		queue_insert_generic(&irqmsg_queue,self,irqmsg_);
	}
	else if(!mask && oldmask)
	{
		queue_remove_generic(self,irqmsg_);
	}

	spinlock_release(&irqmsg_queue_spinlock);
	level_return(level);

	// now adjust the handler counts
	
	// first the added handlers
	maskdiff = oldmask ^ mask; // changed bits
	maskdiff &= mask; // changed from 0 to 1

	while(maskdiff)
	{
		i = bits_lowest_set(maskdiff);
		maskdiff &= ~(1 << i);

		if(atomic_add(&irqmsg_handler_count[i],1)==0)
			int_enable_irq(i);
	}

	// now the removed handlers
	maskdiff = oldmask ^ mask; // changed bits
	maskdiff &= oldmask; // changed from 1 to 0

	while(maskdiff)
	{
		i = bits_lowest_set(maskdiff);
		maskdiff &= ~(1 << i);

		if(atomic_sub(&irqmsg_handler_count[i],1)==1)
			int_disable_irq(i);
	}

	return 0;
}

void irqmsg_sendirq(int irq)
{
	process *p;
	int level;
	int mask;

	// construct mask
	mask = 1 << irq;

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);
	spinlock_grab(&irqmsg_queue_spinlock);

	// iterate through list of waiting processes, checking if any are waiting
	// on this irq
	for(p=irqmsg_queue.head;p;p=p->irqmsg_next)
	{
		if( ! (p->irqmsg_mask & mask) )
			continue;

		// found one - set pending bit, and put on runqueue if it's waiting
		// vector
		p->irqmsg_pending |= mask;

		if(p->msgstate == PROCESS_MSGSTATE_REC)
		{
			queue_insert(&process_queues[p->prio],p);
			process_queues_runnable_mask |= (1 << p->prio);
		}
	}

	spinlock_release(&irqmsg_queue_spinlock);
	spinlock_release(&process_spinlock);
	level_return(level);
}
