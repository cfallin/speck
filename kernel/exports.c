/*
 * kernel/exports.c
 */

#include <kernel/exports.h>

#include <kernel/malloc.h>
#include <kernel/level.h>
#include <kernel/msg.h>
#include <kernel/irqmsg.h>
#include <kernel/process.h>
#include <kernel/sem.h>
#include <kernel/mutex.h>
#include <kernel/arch/lock.h>
#include <kernel/arch/panic.h>
#include <kernel/arch/vid.h>
#include <kernel/arch/string.h>

#define f(name) { ((int)(name)), # name }
#define data(name) { ((int)(&(name))), # name }

kernel_export kernel_exports[] =
{
	f(malloc),
	f(free),
	f(level_go),
	f(level_return),
	f(msg_send),
	f(msg_recv),
	f(msg_reply),
	f(irqmsg_register),
	f(process_create),
	f(process_kill),
	f(process_exit),
	f(sem_init),
	f(sem_free),
	f(sem_up),
	f(sem_down),
	f(mutex_init),
	f(mutex_free),
	f(mutex_lock),
	f(mutex_unlock),
	f(atomic_add),
	f(atomic_sub),
	f(atomic_test_and_set),
	f(atomic_xchg),
	f(spinlock_grab),
	f(spinlock_release),
	f(panic),
	f(vid_puts),
	f(vid_putc),
	f(vid_putd),
	f(vid_puthex),
	f(memcpy),
	f(memset),
	f(strcmp),
	f(strncmp),
	f(strcpy),
	f(strncpy),
	f(strlen),
};

int kernel_export_count = sizeof(kernel_exports) / sizeof(kernel_export);
