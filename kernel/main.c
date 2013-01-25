/*
 * kernel/main.c
 */

#include <kernel/arch/init.h>
#include <kernel/mm.h>
#include <kernel/process.h>
#include <kernel/kmod.h>
#include <kernel/arch/vid.h>
#include <kernel/arch/int.h>
#include <version.h>

void init_process();
void idle_process();

int kernel_main()
{
	arch_init();
	mm_init();
	process_init();
	kmod_init();

	vid_puts("\nspeck/" SPECK_ARCH " version " SPECK_VERSION "\n");
	vid_puts("  Built " SPECK_BUILD_DATE " by " SPECK_BUILD_USER "@"
		SPECK_BUILD_HOST "\n");

	vid_puts("\nDone initializing.\n");

	// TODO: one idle process per processor
	process_create((int)idle_process,0,0,1);

	// enable interrupts
	int_enable(INT_ENABLE_FLAGS_INIT);

	while(1);
}

void idle_process()
{
	while(1);
}

void init_process()
{
	vid_puts("Initial process started\n");

	vid_puts("Loading boot modules...\n");
	kmod_arch_boot_modules();

	while(1); // DON'T RETURN
}
