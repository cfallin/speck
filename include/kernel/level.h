/*
 * include/kernel/level.h
 */

#ifndef _KERNEL_LEVEL_H_
#define _KERNEL_LEVEL_H_

#define LEVEL_USER      0
#define LEVEL_KERNEL    1
#define LEVEL_NOPREEMPT 2
#define LEVEL_NOINTS    3

#include <kernel/arch/smp.h>

extern int cpu_levels[SMP_MAX_CPUS];
extern int cpu_int_restore_flags[SMP_MAX_CPUS];

int level_go(int newlevel);
int level_return(int newlevel);

#endif
