/*
 * kernel/process.c
 */

#include <kernel/process.h>
#include <kernel/mm.h>
#include <kernel/malloc.h>
#include <kernel/arch/string.h>
#include <kernel/arch/smp.h>
#include <kernel/arch/lock.h>
#include <kernel/level.h>
#include <kernel/queue.h>
#include <kernel/arch/vid.h>
#include <kernel/arch/bits.h>
#include <kernel/arch/resched.h>
#include <kernel/arch/int.h>
#include <kernel/arch/panic.h>

process *process_current[SMP_MAX_CPUS];
process *process_pid_table[PROCESS_PID_MAX];

process_queue process_queues[PROCESS_MAX_PRIO];
int process_queues_runnable_mask;

int process_spinlock;

int init_process_pid;

void init_process();

void process_init()
{
	init_process_pid = process_create((int)(init_process),0,8,1);

	vid_puts("Initial process created: pid ");
	vid_putd(init_process_pid);
	vid_putc('\n');
}

pid_t process_create(int pc, int sp, int prio, int is_kthread)
{
	int level;
	int pid;
	process *p;
	process_queue *q;

	// allocate a process structure
	p = malloc(sizeof(process));
	memset(p,0,sizeof(process));

	// grab a process slot
	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	for(pid = 0; pid < PROCESS_PID_MAX; pid++)
	{
		if(!process_pid_table[pid])
			break;
	}
	
	if(pid == PROCESS_PID_MAX)
	{
		spinlock_release(&process_spinlock);
		level_return(level);
		free(p);

		return -1;
	}

	process_pid_table[pid] = p;
	p->pid = pid;

	spinlock_release(&process_spinlock);
	level_return(level);

	// initialize the context
	if(is_kthread)
		context_create_kernel(&p->ctx,pc);
	else
		context_create_user(&p->ctx,pc,sp);

	// set the priority and kthread status
	p->prio = prio;
	p->is_kthread = is_kthread;

	// set the preferred CPU - starts out as "none"
	p->cpu = -1;

	// put in a run queue
	q = &process_queues[prio];

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	queue_insert(q,p);
	process_queues_runnable_mask |= (1 << prio);

	// process can now safely receive messages
	p->msgstate = PROCESS_MSGSTATE_READY;

	spinlock_release(&process_spinlock);
	level_return(level);

	// done
	return pid;
}

pid_t process_fork()
{
	return -1;
}

int process_kill(pid_t pid)
{
	process *p;
	int level;

	if(pid < 0 || pid > PROCESS_PID_MAX)
		return -1;

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	// find the process
	p = process_pid_table[pid];
	if(!p)
	{
		spinlock_release(&process_spinlock);
		level_return(level);
		return -1;
	}

	// kernel threads can't be killed from another proc
	if(p->is_kthread && process_current[smp_cpu_id()]->pid != pid)
	{
		spinlock_release(&process_spinlock);
		level_return(level);
		return -1;
	}

	// set the kill-flag (the process is freed in the scheduler)
	p->exit_flag = 1;

	spinlock_release(&process_spinlock);
	level_return(level);

	// if we just killed ourselves, resched to let the scheduler do its work
	if(pid == process_current[smp_cpu_id()]->pid)
		while(1)
			resched();

	return 0;
}

int process_exit(int exitcode)
{
	return process_kill(process_current[smp_cpu_id()]->pid);
}

void process_do_delete(process *p)
{
	// delete a process - p shouldn't be on a queue, and should be at user
	// level

	// TODO: handle processes waiting to send to or waiting for a reply from 
	// this process

	// TODO: handle when this process is waiting on a send

	process_pid_table[p->pid] = NULL;

	context_free(&p->ctx);
	free(p);
}

context *process_schedule()
{
	process *p;
	int prio;
	int level;

	// don't preempt if we're at or above LEVEL_NOPREEMPT
	if(cpu_levels[smp_cpu_id()] >= LEVEL_NOPREEMPT)
		return &process_current[smp_cpu_id()]->ctx;

	// make sure we're not interrupted
	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&process_spinlock);

	// find a process to run

	// if nothing to run then just return
	if(!process_queues_runnable_mask)
	{
		// make sure the currently running process still wants to run
		if(process_current[smp_cpu_id()]->msgstate == PROCESS_MSGSTATE_READY)
		{
			spinlock_release(&process_spinlock);
			level_return(level);
			return &process_current[smp_cpu_id()]->ctx;
		}
		else
		{
			// if not, then we truly have nothing to run, and we can't go back
			// to the old process - in this case, when all else fails, panic
			// :-)
			panic("NO RUNNABLE PROCESSES AND CURRENT PROCESS IS NOT RUNNABLE");
		}
	}

	// put the currently-running process back on the run queue if it hasn't
	// been put somewhere else already
	// we need to do this here so that it has a chance of being chosen below
	// (if it's the highest-priority process)
	if(process_current[smp_cpu_id()] && !process_current[smp_cpu_id()]->queue &&
		process_current[smp_cpu_id()]->msgstate == PROCESS_MSGSTATE_READY)
	{
		process *oldproc = process_current[smp_cpu_id()];

		oldproc->running = 0;
		oldproc->level = level;

		queue_remove(oldproc);
		queue_insert(&process_queues[oldproc->prio],oldproc);
		process_queues_runnable_mask |= (1 << oldproc->prio);
	}

restart_sched:
	// highest priority with a runnable process
	prio = bits_highest_set(process_queues_runnable_mask);
	if(!process_queues_runnable_mask)
		panic("NO RUNNABLE PROCESSES");

	// take the process off the run queue
	p = process_queues[prio].head;
	queue_remove(p);

	// if the queue is now empty, take note of that
	if(!process_queues[prio].head)
		process_queues_runnable_mask &= ~(1 << prio);

	// if the process has the delete flag set, delete it
	if(p->exit_flag && (p->level == LEVEL_USER))
	{
		spinlock_release(&process_spinlock);
		level_return(level);

		process_do_delete(p);

		level = level_go(LEVEL_NOINTS);
		spinlock_grab(&process_spinlock);

		goto restart_sched;
	}

	// set the process state as "running"
	p->running = 1;
	p->cpu = smp_cpu_id();
	process_current[smp_cpu_id()] = p;

	// release locks
	spinlock_release(&process_spinlock);
	level_return(level);

	// return new context
	return &p->ctx;
}
