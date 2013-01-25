/*
 * include/kernel/i386/paging.h
 */

#ifndef _KERNEL_I386_PAGING_H_
#define _KERNEL_I386_PAGING_H_

#include <kernel/i386/types.h>
#include <kernel/mm.h>

typedef u32 addr_t;

#define PAGING_KERNEL_START  0
#define PAGING_KERNEL_END    0x0f800000
#define PAGING_SELFPD        ((volatile addr_t *)0x0f800000)
#define PAGING_SELFPD_PDE    62
#define PAGING_SELFPDIR      ((volatile addr_t *)0x0f83e000)
#define PAGING_PD2           ((volatile addr_t *)0x0fc00000)
#define PAGING_PD2_PDE       63
#define PAGING_PD2PDIR       ((volatile addr_t *)0x0fc3f000)
#define PAGING_PHYSMEM_START 0x10000000
#define PAGING_PHYSMEM_END   0x80000000
#define PAGING_USER_START    0x80000000
#define PAGING_USER_END      0xffffffff

#define PAGE_ROUNDUP(x) ( ((x)+4095)&0xfffff000 )
#define PAGE_ROUNDDOWN(x) ( (x)&0xfffff000 )

// physmem stuff
void physmem_init();
void physmem_uninit();

// general init
void paging_init(mm_phys_page_pool *pool);

// pagefault handler
void paging_pagefault(addr_t addr, int errcode);

// address conversion
addr_t paging_phys_to_virt(addr_t phys);
addr_t paging_virt_to_phys(addr_t virt);

// address spaces
typedef struct _aspace
{
	int refcount;
	addr_t pdir;
	addr_t ptab_refcounts[512];
} aspace;
typedef struct aspace *aspace_t;

aspace_t paging_aspace_create();
void     paging_aspace_ref(aspace_t aspace);
void     paging_aspace_release(aspace_t aspace);
aspace_t paging_aspace_clone(aspace_t aspace, int cow);

// page mapping

#define PAGING_PRESENT  1
#define PAGING_WRITE    2
#define PAGING_USER     4
#define PAGING_ACCESSED 0x20
#define PAGING_DIRTY    0x40
// first user bit in PTE - kspace commit-on-access
#define PAGING_KSP_COMMIT 0x200
#define PAGING_PAGESHIFT 12

int paging_page_map(aspace_t aspace, addr_t virt, addr_t phys, int prot);
int paging_page_get_mapping(aspace_t aspace, addr_t virt);

// kernel space

typedef struct _kspace_page
{
	struct _kspace_page *prev, *next;
	int size;
} kspace_page;
extern kspace_page *kspace_pages;
extern kspace_page *kspace_head;

#define KSPACE_COMMIT_ON_ACCESS 1 // commit a page on first access
#define KSPACE_COMMIT_NOW 2 // commit all pages of region immediately
#define KSPACE_VIRT_ONLY 3 // alloc virtual space only

#define KSPACE_GRANULARITY 256 // granularity of allocation, in pages - 1MB
#define KSPACE_GRAN_MASK 0xffffff00

void *kspace_alloc(int npages, int type);
void  kspace_free (void *addr);

// symbols defined in linker script
extern int _stext, _etext, _srodata, _erodata, _sdata, _edata, _sbss, _ebss;

// macros
#define invlpg(x) asm volatile("invlpg (%0)"::"r" (x))
#define invltlb() asm volatile("movl %%cr3, %%eax; movl %%eax, %%cr3":::"eax")

#endif
