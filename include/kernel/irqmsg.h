/*
 * include/kernel/irqmsg.h
 */

#ifndef _KERNEL_IRQMSG_H_
#define _KERNEL_IRQMSG_H_

#include <kernel/process.h>

extern process_queue irqmsg_queue;

#define IRQMSG_IRQ(x) (1 << (x))

int irqmsg_register(int irqmask);

void irqmsg_sendirq(int irq);

#endif
