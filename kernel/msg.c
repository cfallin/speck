/*
 * kernel/msg.c
 */

#include <kernel/msg.h>
#include <kernel/process.h>
#include <kernel/level.h>
#include <kernel/queue.h>
#include <kernel/arch/lock.h>
#include <kernel/arch/string.h>
#include <kernel/arch/resched.h>
#include <kernel/arch/panic.h>
#include <kernel/arch/bits.h>

int msg_send(pid_t proc, char *sndbuf, int sndlen, char *recbuf, int *reclen)
{
	int level;
	process *self, *rec;

	// check the target pid
	if(proc < 0 || proc > PROCESS_PID_MAX)
		return -1;

	// set msg params in our process struct
	self = process_current[smp_cpu_id()];
	self->sndbuf = sndbuf;
	self->sndlen = sndlen;
	self->recbuf = recbuf;
	self->reclen = *reclen;

	// enter critical section now
	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	// get target process
	rec = process_pid_table[proc];
	if(!rec || rec->msgstate == 0)
	{
		spinlock_release(&process_spinlock);
		level_return(level);
		return -1;
	}

	// avoid a deadlock (A sends to B while B is blocking on send to A)
	if(rec->msgstate == PROCESS_MSGSTATE_SEND && (rec->queue == &self->sndqueue
		|| rec->queue == &self->replyqueue))
	{
		spinlock_release(&process_spinlock);
		level_return(level);
		return -1;
	}

	// set our msgstate
	self->msgstate = PROCESS_MSGSTATE_SEND;

	// now enqueue ourselves on the sendqueue of the target proc
	queue_insert(&rec->sndqueue,self);

	// check target process's state
	if(rec->msgstate == PROCESS_MSGSTATE_REC)
	{
		// the target process is waiting to receive - wake them up
		queue_insert(&process_queues[rec->prio],rec);
		process_queues_runnable_mask |= (1 << rec->prio);
		rec->msgstate = PROCESS_MSGSTATE_READY;
	}

	// the target is now executing, one way or another - just sleep; 
	// they'll see us on the queue and wake us up once we get a reply

	spinlock_release(&process_spinlock);
	level_return(level);

	resched();

	// we have returned, and gotten a reply; set *reclen to the reply length
	*reclen = self->reclen;

	// done
	return 0;
}

#define min(a,b) ((a < b) ? (a) : (b))

int msg_recv(pid_t *sender, char *buf, int *buflen)
{
	int level;
	process *self, *s;

	self = process_current[smp_cpu_id()];

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	// check for a sender first
	while(!self->sndqueue.head && !self->irqmsg_pending)
	{
		// no sender - sleep
		self->msgstate = PROCESS_MSGSTATE_REC;

		spinlock_release(&process_spinlock);
		level_return(level);

		resched();

		level = level_go(LEVEL_NOINTS);
		spinlock_grab(&process_spinlock);
	}

	// we have a sender

	// first set our state to "ready"
	self->msgstate = PROCESS_MSGSTATE_READY;

	// handle IRQ messages
	if(self->irqmsg_pending)
	{
		int vector;

		// find lowest pending IRQ
		vector = bits_lowest_set(self->irqmsg_pending);

		// unset that pending bit
		self->irqmsg_pending &= ~(1 << vector);

		// return the vector in the "sender" field
		*sender = vector;

		// done
		spinlock_release(&process_spinlock);
		level_return(level);

		return MSG_TYPE_IRQ;
	}

	// first dequeue them
	s = self->sndqueue.head;
	queue_remove(s);

	// copy the message
	*buflen = min(*buflen,s->sndlen);
	memcpy(buf,s->sndbuf,*buflen);

	// save the sender's PID
	*sender = s->pid;

	// set the sender's state to "waiting for reply"
	s->msgstate = PROCESS_MSGSTATE_REPLY;

	// enqueue the sender on the reply queue
	queue_insert(&self->replyqueue,s);

	// done
	spinlock_release(&process_spinlock);
	level_return(level);

	return MSG_TYPE_IPC;
}

int msg_reply(pid_t proc, char *buf, int len)
{
	int level;
	process *self, *p;

	if(proc < 0 || proc > PROCESS_PID_MAX)
		return -1;

	self = process_current[smp_cpu_id()];

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	// make sure the dest proc is actually waiting for a reply
	if(!process_pid_table[proc] || process_pid_table[proc]->queue != 
		&self->replyqueue)
	{
		spinlock_release(&process_spinlock);
		level_return(level);

		return -1;
	}

	p = process_pid_table[proc];

	// copy the reply
	p->reclen = min(p->reclen, len);
	memcpy(p->recbuf,buf,p->reclen);

	// put the process on the run queue
	p->msgstate = PROCESS_MSGSTATE_READY;
	queue_remove(p);
	queue_insert(&process_queues[p->prio],p);
	process_queues_runnable_mask |= (1 << p->prio);

	spinlock_release(&process_spinlock);
	level_return(level);

	// done - reschedule
	resched();

	return 0;
}
