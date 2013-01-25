/*
 * include/kernel/i386/int.h
 */

#ifndef _KERNEL_I386_INT_H_
#define _KERNEL_I386_INT_H_

#include <kernel/i386/context.h>

void int_init();

int int_handler_dispatch(int current_esp);

#define INT_MAX_IRQS 16

extern char idt[2048];

#define INT_ENABLE_FLAGS_INIT 0x202 // flags for init process

int int_disable();
void int_enable(int flags);

void int_disable_irq(int irq);
void int_enable_irq(int irq);

#endif
