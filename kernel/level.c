/*
 * kernel/level.c
 */

#include <kernel/level.h>
#include <kernel/arch/lock.h>
#include <kernel/arch/int.h>
#include <kernel/arch/panic.h>
#include <kernel/arch/vid.h>

int cpu_levels[SMP_MAX_CPUS];
int cpu_int_restore_flags[SMP_MAX_CPUS];

int level_go(int newlevel)
{
	int oldlevel;
	int flags;

	// we can't take an interrupt in this routine
	flags = int_disable();

	// because level is per-CPU, as long as ints are disabled we don't need
	// locking
	
	oldlevel = cpu_levels[smp_cpu_id()];
	
	// error checking
	if(newlevel < oldlevel)
	{
		void vid_puts_internal(char *s);

		char buf[2];
		buf[1] = 0;
		buf[0] = '\n';
		vid_puts_internal(buf);
		buf[0] = '0' + oldlevel;
		vid_puts_internal(buf);
		buf[0] = '0' + newlevel;
		vid_puts_internal(buf);
		buf[0] = '\n';
		vid_puts_internal(buf);
		vid_puthex((int)__builtin_return_address(0));
		panic("\nattempting to lower CPU level without going through level_return");
	}

	// save flags (with previous interrupt bit state) if we're going to level
	// NOINTS
	if(newlevel == LEVEL_NOINTS && oldlevel != LEVEL_NOINTS)
		cpu_int_restore_flags[smp_cpu_id()] = flags;

	// now set the new level
	cpu_levels[smp_cpu_id()] = newlevel;

	// if we're not at level NOINTS, restore interrupt flag
	if(newlevel != LEVEL_NOINTS)
		int_enable(flags);

	return oldlevel;
}

int level_return(int newlevel)
{
	int flags;
	int oldlevel;

	// locking
	flags = int_disable();

	// we don't need locking (spinlocks etc) here - see above
	
	oldlevel = cpu_levels[smp_cpu_id()];

	// error checking
	if(newlevel > oldlevel)
		panic("attempting to raise CPU level with level_return");

	// set the new level
	cpu_levels[smp_cpu_id()] = newlevel;

	// restore flags
	if(newlevel < LEVEL_NOINTS)
	{
		if(oldlevel == LEVEL_NOINTS)
			int_enable(cpu_int_restore_flags[smp_cpu_id()]);
		else
			int_enable(flags);
	}

	// done
	return oldlevel;
}
