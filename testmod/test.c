/*
 * testmod/test.c
 */

#include <kernel/malloc.h>
#include <kernel/arch/vid.h>
#include <kernel/process.h>

// test .data relocations
void *(*malloc_test)(int size) = &malloc;

int test()
{
	void *p;

	p = malloc_test(100);

	// test normal .text relocations
	vid_puts("testmod loaded!\n");
	vid_puts("test: ");
	vid_puthex((int)p);
	vid_putc('\n');

	return 0;
}

// called on load
void kmod_init()
{
	// test intra-module relocations
	test();
}
