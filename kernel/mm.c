/*
 * kernel/mm.c
 */

#include <kernel/arch/paging.h>
#include <kernel/mm.h>
#include <kernel/malloc.h>
#include <kernel/arch/panic.h>
#include <kernel/arch/lock.h>
#include <kernel/arch/int.h>

mm_phys_page_pool mm_phys_pages;
int mm_phys_pages_lock = 0;

void mm_init()
{
	physmem_init();
	paging_init(&mm_phys_pages);
	malloc_init();
}

void mm_pagefault(int addr, int type)
{
}

int mm_physpage_alloc(int type, int allowblock)
{
	mm_phys_page *p;
	int flags;

	flags = int_disable();
	spinlock_grab(&mm_phys_pages_lock);

	if(!mm_phys_pages.head)
	{
		// TODO: implement page stealing
		panic("Out of physical memory");
	}

	p = mm_phys_pages.head;
	mm_phys_pages.head = p->next;
	p->type = type;
	p->refcount = 1;

	mm_phys_pages.free_pagecount--;

	spinlock_release(&mm_phys_pages_lock);
	int_enable(flags);

	return p->physaddr;
}

void mm_physpage_free(int page)
{
	mm_phys_page *p;
	int flags;

	p = &(mm_phys_pages.pages[page]);

	if(atomic_sub(&(p->refcount),1) > 0)
		return;

	flags = int_disable();
	spinlock_grab(&mm_phys_pages_lock);

	p->next = mm_phys_pages.head;
	mm_phys_pages.head = p;

	mm_phys_pages.free_pagecount++;

	p->type = PHYSPAGE_FREE;
	p->refcount = 0;

	spinlock_release(&mm_phys_pages_lock);
	int_enable(flags);
}
