/*
 * include/kernel/process.h
 */

#ifndef _KERNEL_PROCESS_H_
#define _KERNEL_PROCESS_H_

#include <kernel/arch/context.h>

typedef int pid_t;

typedef struct process process;
typedef struct process_queue process_queue;

struct process_queue
{
	process *head, *tail;
};

struct process
{
	context ctx;
	// TODO: mm_aspace asp;
	pid_t pid;

	int prio;
	int is_kthread;

	int cpu;
	int running;
	int level; // valid if !running

	int exit_flag;

	// stuff for messages
	char *sndbuf;
	int sndlen;
	char *recbuf;
	int reclen;
	int msgstate;
	process_queue sndqueue;
	process_queue replyqueue;

	// stuff for irq messages
	int irqmsg_mask;
	int irqmsg_pending;
	process_queue *irqmsg_queue;
	process *irqmsg_prev, *irqmsg_next;

	process_queue *queue;
	process *prev, *next;
};

#define PROCESS_MSGSTATE_READY 1
#define PROCESS_MSGSTATE_REC   2
#define PROCESS_MSGSTATE_SEND  3
#define PROCESS_MSGSTATE_REPLY 4

void process_init();

// creates a process sharing the current aspace
pid_t process_create(int pc, int sp, int prio, int is_kthread);
// POSIX fork()-like syscall
pid_t process_fork();

int process_kill(pid_t pid);
int process_exit(int exitcode);

context *process_schedule();

#include <kernel/arch/smp.h>

#define PROCESS_PID_MAX 1024

extern process *process_current[SMP_MAX_CPUS];
extern process *process_pid_table[PROCESS_PID_MAX];

#define PROCESS_MAX_PRIO 32

extern process_queue process_queues[PROCESS_MAX_PRIO];
extern int process_queues_runnable_mask;

extern int process_spinlock;

#endif
