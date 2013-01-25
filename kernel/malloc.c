/*
 * kernel/malloc.c
 */

#include <kernel/malloc.h>
#include <kernel/bget.h>
#include <kernel/arch/paging.h>
#include <kernel/arch/string.h>
#include <kernel/arch/lock.h>
#include <kernel/arch/vid.h>
#include <kernel/mutex.h>

mutex malloc_lock;

void malloc_init()
{
	// initialize the bget allocator

	bectl(
		NULL, // compaction callback
		malloc_bget_acquire_callback, // alloc callback
		malloc_bget_release_callback, // free callback
		1048576 // heap-expansion increment
	);

	mutex_init(&malloc_lock);
}

void *malloc(int size)
{
	void *p;
	
	mutex_lock(&malloc_lock);
	p = bget(size);
	mutex_unlock(&malloc_lock);

	return p;
}

void free(void *p)
{
	mutex_lock(&malloc_lock);
	brel(p);
	mutex_unlock(&malloc_lock);
}

// bget callbacks
void *malloc_bget_acquire_callback(long size)
{
	// round size up to the nearest page
	size = (size + 4095) >> 12;

	// allocate a kspace block
	// we must pre-allocate all pages - what if code critical to freeing up
	// memory is trying to do its job while almost no pages are free, and it
	// touches a not-yet-committed block of kernel memory?
	return kspace_alloc(size,KSPACE_COMMIT_NOW);
}

void malloc_bget_release_callback(void *buf)
{
	// this one's easy: just pass directly to kspace layer
	kspace_free(buf);
}
