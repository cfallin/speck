/*
 * include/kernel/mm.h
 */

#ifndef _KERNEL_MM_H_
#define _KERNEL_MM_H_

typedef struct mm_phys_page_pool mm_phys_page_pool;
typedef struct mm_cache mm_cache;
typedef struct mm_phys_page mm_phys_page;
typedef struct mm_virt_page mm_virt_page;
typedef struct mm_store mm_store;
typedef struct mm_store_ops mm_store_ops;
typedef struct mm_region mm_region;
typedef struct mm_aspace mm_aspace;

#define PHYSPAGE_FREE 0
#define PHYSPAGE_USER 1
#define PHYSPAGE_KERNEL 2
#define PHYSPAGE_CACHE  3
#define PHYSPAGE_RSVD 255;

struct mm_phys_page
{
	// for linked lists
	mm_phys_page *next;
	int physaddr;

	int refcount;
	int type;

	// for cache pages
	int blockaddr;
	mm_store *store;
};

struct mm_phys_page_pool
{
	mm_phys_page *pages;
	int pagecount;
	mm_phys_page *head;
	int free_pagecount;
};

extern mm_phys_page_pool mm_phys_pages;

#define MM_CACHE_HASH(store,blockaddr) \
({ \
	int baddr_low = ((blockaddr)&0xffffffff), baddr_high = \
		((blockaddr)&0xffffffff00000000LL); \
	int s = (int)(store); \
	\
	((baddr_low ^ baddr_high ^ s) + (s & 0xffff) + (baddr_low & 0xffff)) \
		& 0xfff; \
})

#define MM_CACHE_HASH_MAX 0x1000

struct mm_cache
{
	mm_phys_page *pages[MM_CACHE_HASH_MAX];
};

#define VIRTPAGE_FLAG_COW 1
#define VIRTPAGE_FLAG_LOCKED 2

// where actual page is located
#define VIRTPAGE_PHYS   1 // in physical memory
#define VIRTPAGE_STORED 2 // stored on a mass storage device
#define VIRTPAGE_ANON   3 // allocate physical page on access
#define VIRTPAGE_ZEROED 4 // allocate zeroed physical page on access (BSS)

struct mm_virt_page
{
	short flags;

	// where the page is physically
	
	
	// info about the page's location
	union
	{
		// mass storage
		struct
		{
			mm_store *store;
			int block;
		} stored;

		// in physical memory
		mm_phys_page *physpage;
	} loc;
};

#define PAGE_READ    1
#define PAGE_WRITE   2
#define PAGE_EXECUTE 4

struct mm_region
{
	int addr, size;

	// read, write, execute, etc.
	short perms;

	mm_virt_page *pages;

	mm_region *prev, *next;
};

struct mm_store
{
	mm_store_ops *ops;
	void *cookie;
};

struct mm_store_ops
{
	int refcount;

	int (*load_page)(int loc, int phys);
	int (*write_page)(int loc, int phys);
};

struct mm_aspace
{
};

void mm_init();

void mm_pagefault(int addr, int type);

int mm_physpage_alloc(int type, int allowblock);
void mm_physpage_free(int page);

#endif
