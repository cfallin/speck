/*
 * kernel/i386/int.c
 */

#include <kernel/i386/int.h>
#include <kernel/i386/int_handler.h>
#include <kernel/i386/context.h>
#include <kernel/i386/paging.h>
#include <kernel/i386/vid.h>
#include <kernel/i386/lock.h>
#include <kernel/i386/io.h>
#include <kernel/i386/panic.h>
#include <kernel/level.h>
#include <kernel/process.h>
#include <kernel/irqmsg.h>

int int_mask_master;
int int_mask_slave;

void int_init()
{
	int_handler_init();

	int_mask_master = int_mask_slave = 0xff;
	int_enable_irq(0);
	int_enable_irq(2);
}

// used for printing hex values when we must panic without using locks
void vid_puthex_internal(int n, int locks);

int int_handler_dispatch(int current_esp)
{
	stack_context *ctx = (stack_context *)current_esp;
	context *newproc;
	int oldlevel;

	// catch fatal pagefaults in non-preemptable sections
	if(ctx->vector == 14 && cpu_levels[smp_cpu_id()] > LEVEL_KERNEL)
	{
		int cr2;

		// get cr2
		asm volatile("movl %%cr2, %0" : "=r" (cr2) :: "memory");

		vid_puthex_internal(cr2,0);
		vid_puthex_internal(ctx->eip,0);

		panic("FATAL: PAGEFAULT IN NON-PREEMPTABLE KERNEL SECTION");
	}

	// catch all other interrupts from (supposedly) non-interruptable sections
	if(cpu_levels[smp_cpu_id()] >= LEVEL_NOINTS)
	{
		vid_puthex_internal(ctx->eip,0);
		panic("FATAL: INTERRUPT/EXCEPTION IN NON-INTERRUPTABLE KERNEL SECTION");
	}

	// locking - don't try to "raise" the level to KERNEL when we've already
	// interrupted a NOPREEMPT section (note that NOPREEMPT is different than
	// NOINTS)
	if(cpu_levels[smp_cpu_id()] == LEVEL_NOPREEMPT)
		oldlevel = LEVEL_NOPREEMPT;
	else
		oldlevel = level_go(LEVEL_KERNEL);

	// dispatch interrupt
	if(ctx->vector == 14) // page fault
	{
		int cr2;

		// get cr2
		asm volatile("movl %%cr2, %0" : "=r" (cr2) :: "memory");

		// now that we have cr2, we can re-enable interrupts (we entered via
		// an interrupt gate, so they were disabled)
		asm("sti");

		// call handler
		paging_pagefault(cr2, ctx->errcode);
	}
	else if((ctx->vector >= 0x20) && (ctx->vector < 0x30))
	{
		// IRQ

		// send IRQ message - this will wake up all procs waiting for this IRQ
		irqmsg_sendirq(ctx->vector - 0x20);
	}
	else if(ctx->vector == 0x80)
	{
		// syscall
	}
	else if(ctx->vector == 0x81)
	{
		// reschedule - this handler intentionally left blank; we just let the
		// scheduler do its work below
	}
	else
	{
		vid_puts("\n\nFatal exception ");
		vid_puthex(ctx->vector);
		vid_puts(" eip ");
		vid_puthex(ctx->eip);
		vid_puts(" errcode ");
		vid_puthex(ctx->errcode);
		asm("cli;hlt");
	}

	// return to previous level
	if(oldlevel < LEVEL_NOPREEMPT)
		level_return(oldlevel);

	// from this point on, ensure that we are not interrupted - if the data
	// structures indicate that we are not on this stack, but we still are,
	// and interrupting code kills this stack, bad things could happen
	asm volatile("cli");
	cpu_int_restore_flags[smp_cpu_id()] &= ~0x200;

	// call the scheduler 
	if(process_current[smp_cpu_id()])
		process_current[smp_cpu_id()]->ctx.sp = (void *)current_esp;
	newproc = process_schedule();

	// send EOI
	if((ctx->vector >= 0x20) && (ctx->vector < 0x30))
	{
		if(ctx->vector > 0x27)
		{
			// IRQ is on slave PIC
			outb(0x60 | (ctx->vector - 0x28),0xa0); // specific EOI to slave
			outb(0x62, 0x20); // EOI IRQ 2 (the cascade IRQ) on master
		}
		else
			outb(0x60 | (ctx->vector - 0x20),0x20); // specific EOI to master
	}


	return (int)newproc->sp;
}

int int_disable()
{
	int flags;

	asm volatile
	(
	 "pushfl\n"
	 "popl %0\n"
	 "cli\n"
	 : "=r" (flags)
	);

	return flags;
}

void int_enable(int flags)
{
	asm volatile
	(
	 "pushl %0\n"
	 "popfl\n"
	 :: "r" (flags)
	);
}

void int_disable_irq(int irq)
{
	// never disable the cascade line
	if(irq == 2)
		return;

	// never disable the timer
	if(irq == 0)
		return;

	if(irq > 7)
	{
		int_mask_slave |= (1 << (irq - 8));
		outb(int_mask_slave,0xa1);
	}
	else
	{
		int_mask_master |= (1 << irq);
		outb(int_mask_master,0x21);
	}
}

void int_enable_irq(int irq)
{
	if(irq > 7)
	{
		int_mask_slave &= ~(1 << (irq - 8));
		outb(int_mask_slave,0xa1);
	}
	else
	{
		int_mask_master &= ~(1 << irq);
		outb(int_mask_master,0x21);
	}
}
