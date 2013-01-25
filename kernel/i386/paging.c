/*
 * kernel/i386/paging.c
 */

#include <kernel/i386/paging.h>
#include <kernel/i386/string.h>
#include <kernel/i386/multiboot.h>
#include <kernel/i386/panic.h>
#include <kernel/i386/vid.h>
#include <kernel/i386/lock.h>
#include <kernel/i386/int.h>
#include <kernel/level.h>

kspace_page *kspace_head;
kspace_page *kspace_pages;
int *kspace_ptabs[PAGING_KERNEL_END >> 22];
int kspace_lock = 0;

int physmem_page_count;

int paging_first_free_phys()
{
	struct mod_list *mods;
	int count,i;
	unsigned int highest_addr = (unsigned int)&_ebss;

	// if no modules present then return
	if(!mboot_valid) goto out;
	if(!(mboot_info->flags & MB_INFO_MODS)) goto out;
	
	// get info
	mods = (struct mod_list *)mboot_info->mods_addr;
	count = mboot_info->mods_count;
	
	// find highest address occupied by module
	for(i=0;i<count;i++)
	{
		if(mods[i].mod_start < (unsigned int)&_ebss)
		{
			vid_puts("mod at ");
			vid_puthex(mods[i].mod_start);
			vid_puts(", end of kernel is ");
			vid_puthex((unsigned int) &_ebss);
			panic("BRAIN-DEAD LOADER STUCK A MODULE ON TOP OF THE KERNEL!\nI'm"
				" lucky I'm still  alive!\n");
		}

		if(mods[i].mod_end > highest_addr)
			highest_addr = mods[i].mod_end;

		vid_puts("Paging: found Multiboot module at addr ");
		vid_puthex(mods[i].mod_start);
		vid_puts(", length ");
		vid_puthex(mods[i].mod_end - mods[i].mod_start);
		vid_putc('\n');
	}

out:
	// round it up
	return (highest_addr + 4095) & 0xfffff000;
}

void paging_init(mm_phys_page_pool *pool)
{
	char *i;
	int *pdir, *ptab;
	int j;
	addr_t freephys,freevirt;

	// figure out amount of memory
	if(mboot_valid && (mboot_info->flags & 1))
	{
		// info from multiboot info block
		physmem_page_count = (mboot_info->mem_upper + 1024) >> 2;
	}
	else
	{
		// HACK - if we weren't booted in a multiboot environment, just assume
		// 4MB memory
		physmem_page_count = 1024;

		// print warning
		vid_puts("Paging: WARNING: no multiboot memory info available, "
			"assuming 4MB RAM\n");
	}

	// first free page
	i = (char *)paging_first_free_phys();

	// reserve area for the physical page frame descriptors
	pool->pages = (mm_phys_page *)i;
	pool->pagecount = physmem_page_count;
	i += (((pool->pagecount * sizeof(mm_phys_page)) + 4095) & 0xfffff000);

	// reserve an area for the kernel space descriptors
	kspace_pages = (kspace_page *)i;
	i += (sizeof(kspace_page)*(PAGING_KERNEL_END >> 12));

	// initialize kspace array
	memset(kspace_pages,0,sizeof(kspace_pages)*(PAGING_KERNEL_END >> 12));
	kspace_head = NULL;

	// init pdir with ident mapping - from 0 to i physical
	pdir = (int *)i;
	ptab = (int *)i; 

	memset(pdir,0,4096); // zero it
	pdir[PAGING_SELFPD_PDE] = (int)pdir | 3; // self mapping

	for(j=4096;j<(int)i;j+=4096)
	{
		// make sure there's a ptab
		if(!pdir[j>>22])
		{
			ptab += 1024;
			memset(ptab,0,4096);
			pdir[j>>22] = (int)ptab | 7;
			kspace_ptabs[j>>22] = ptab;
		}

		// map the page
		ptab[(j>>12)&0x3ff] = j | 3;
	}
	ptab += 1024;

	// first free physical page
	freephys = (int)ptab;
	// first free virtual page (kspace)
	freevirt = (int)i;

	// init kspace array
	kspace_pages[freevirt>>12].size = (PAGING_KERNEL_END-freevirt)>>12;
	kspace_head = &kspace_pages[freevirt>>12];

	// initialize the physpage descriptors
	for(j=0;j<(freephys>>12);j++)
	{
		pool->pages[j].type = PHYSPAGE_RSVD;
		pool->pages[j].refcount = 1;
		pool->pages[j].physaddr = j << 12;
	}
	pool->head = &(pool->pages[j]); // first free physpage
	pool->free_pagecount = pool->pagecount - j; // free page count
	for(;j<pool->pagecount;j++)
	{
		pool->pages[j].type = PHYSPAGE_FREE;
		pool->pages[j].refcount = 0;
		pool->pages[j].physaddr = j << 12;
		pool->pages[j].next = NULL;
		if(j>(freephys>>12))
			pool->pages[j-1].next = &(pool->pages[j]);
	}

	vid_puts("Paging: ");
	vid_putd(pool->free_pagecount << 2);
	vid_puts("KB free mem / ");
	vid_putd(pool->pagecount << 2);
	vid_puts("KB total\n");

	// load CR3
	asm volatile("movl %0, %%cr3" :: "r" ((int)pdir));
	// enable paging
	asm volatile
	(
	 "movl %%cr0, %%eax\n"
	 "orl $0x80010000, %%eax\n" // paging and ring 0 write-protect enable
	 "movl %%eax, %%cr0\n" // do it
	 "jmp 1f\n" // flush cache/pipeline
	 "1:\n"
	 :::"eax"
	);

	// done
}

void paging_pagefault(addr_t addr, int errcode)
{
	if(addr < PAGING_KERNEL_END)
	{
		// kernel heap pagefault

		// check for not-propogated ptab first
		if(!PAGING_SELFPDIR[addr >> 22] && kspace_ptabs[addr >> 22])
		{
			PAGING_SELFPDIR[addr >> 22] = (int)kspace_ptabs[addr >> 22] | 7;
			invltlb();
			return;
		}

		// at this point, there should be a page table present
		if(!PAGING_SELFPDIR[addr >> 22])
			panic("Paging: kernel page fault with non-present ptab");

		// check for kspace commit-on-access
		if(PAGING_SELFPD[addr >> 12] & PAGING_KSP_COMMIT)
		{
			int page;

			page = mm_physpage_alloc(PHYSPAGE_KERNEL,0);

			PAGING_SELFPD[addr >> 12] |= page | 3;
			invlpg(addr);
			return;
		} 
	}

	vid_puts("\nUnknown page fault: cr2 ");
	vid_puthex(addr);
	vid_puts(" errcode ");
	vid_puthex(errcode);
	panic("fatal page fault");
}

void *kspace_alloc(int npages, int type)
{
	kspace_page *p, *leftover;
	int i;
	int level;
	addr_t page;

	// round up requested size
	npages = (npages + KSPACE_GRANULARITY - 1) & KSPACE_GRAN_MASK;

	// don't overcommit memory
	if(npages + 64 > mm_phys_pages.free_pagecount)
		return NULL;

	// TODO: keep track of commited-but-not-yet-allocated page frames

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&kspace_lock);

	// find a block big enough
	for(p = kspace_head; p; p = p->next)
	{
		if(p->size >= npages)
			break;
	}

	// if not enough space then error
	if(!p)
	{
		spinlock_release(&kspace_lock);
		level_return(level);
		return NULL;
	}

	// remove the chunk from free space
	if(p->size > npages)
	{
		// split off leftover block
		leftover = p + npages;

		leftover->size = p->size - npages;
		p->size = npages;

		leftover->prev = p->prev;
		leftover->next = p->next;

		if(leftover->prev)
			leftover->prev->next = leftover;
		else
			kspace_head = leftover;

		if(leftover->next)
			leftover->next->prev = leftover;
	}
	else
	{
		// no leftover block - just remove current one from list
		if(p->prev)
			p->prev->next = p->next;
		else
			kspace_head = p->next;

		if(p->next)
			p->next->prev = p->prev;
	}

	p->next = p->prev = NULL;

	page = p - kspace_pages;

	spinlock_release(&kspace_lock);
	level_return(level);

	// allow preemption here - it's good to split up long critical sections

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&kspace_lock);

	// make sure kernel ptabs are present for area
	for(i=(page >> 10); i < ((((page+npages)+1023)&~1023) >> 10); i++)
	{
		if(!(PAGING_SELFPDIR[i]&1))
		{
			// check if the ptab exists in another aspace
			if(kspace_ptabs[i])
			{
				PAGING_SELFPDIR[i] = (int)kspace_ptabs[i] | 7;
				invltlb();
				continue;
			}

			// alloc a ptab
			PAGING_SELFPDIR[i] = mm_physpage_alloc(PHYSPAGE_KERNEL,0) | 7;
			kspace_ptabs[i] = (int *)(PAGING_SELFPDIR[i] & 0xfffff000);
			invltlb();
		}
	}

	spinlock_release(&kspace_lock);
	level_return(level);

	// map in pages, or set alloc-on-commit flag in PTEs
	if(type == KSPACE_COMMIT_NOW)
		for(i=page;i<(page+npages);i++)
			PAGING_SELFPD[i] = mm_physpage_alloc(PHYSPAGE_KERNEL,0) | 3;
	else if(type == KSPACE_COMMIT_ON_ACCESS)
		for(i=page;i<(page+npages);i++)
			PAGING_SELFPD[i] = PAGING_KSP_COMMIT;
	else if(type == KSPACE_VIRT_ONLY)
		; // nothing

	// done - return address of area
	return (void *)(page << 12);
}

void kspace_free(void *addr)
{
	int page,i;
	kspace_page *p, *prev;
	int level;

	// get page #
	page = (addr_t)addr >> 12;

	// free the pages backing the area, if any
	for(i=page; i < (page+kspace_pages[page].size); i++)
	{
		// skip over this ptab if not present
		if(!PAGING_SELFPDIR[i>>10])
		{
			i = (i + 1023) & ~1023;
			continue;
		}

		// free the page, if any
		if(PAGING_SELFPD[i] & 1)
			mm_physpage_free(PAGING_SELFPD[i] >> 12);

		// unmap the page
		PAGING_SELFPD[i] = 0;
	}

	level = level_go(LEVEL_NOINTS);
	spinlock_grab(&kspace_lock);

	// re-integrate the block into the free list
	p = &kspace_pages[page];
	p->next = p->prev = NULL;

	if((p < kspace_head) || !kspace_head)
	{
		// block is lower than any other free block
		p->next = kspace_head;
		if(kspace_head)
			kspace_head->prev = p;
		kspace_head = p;
	}
	else
	{
		// find location of block in list
		for(prev=kspace_head; prev; prev=prev->next)
		{
			if(!prev->next || (prev->next > p))
				break;
		}

		if(prev->next)
			prev->next->prev = p;
		p->next = prev->next;
		p->prev = prev;
		prev->next = p;
	}

	// merge the block with any consecutive blocks
	if(p->prev && ((p->prev + p->prev->size) == p))
	{
		p->prev->next = p->next;
		if(p->next)
			p->next->prev = p->prev;

		p->prev->size += p->size;
		p = p->prev;
	}
	if(p->next && ((p + p->size) == p->next))
	{
		if(p->next->next)
			p->next->next->prev = p;
		p->size += p->next->size;
		p->next = p->next->next;
	}

	spinlock_release(&kspace_lock);
	level_return(level);
}

void physmem_init()
{
	// nothing
}

void physmem_uninit()
{
	// the OS never "returns" so this is will never be called (this was only
	// originally needed for the i386linux port, and still is for any others 
	// that may be hosted on top of another OS)
}
