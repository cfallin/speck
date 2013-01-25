/*
 * include/kernel/kmod.h
 */

#ifndef _KERNEL_KMOD_H_
#define _KERNEL_KMOD_H_

#include <kernel/mutex.h>

typedef struct kmod_sym kmod_sym;
typedef struct kmod_sym_queue kmod_sym_queue;
typedef struct kmod kmod;
typedef struct kmod_queue kmod_queue;

#define KMOD_SYM_NAME_MAX 32

struct kmod_sym
{
	char name[KMOD_SYM_NAME_MAX];
	int value;
	kmod *mod;

	// hash list
	kmod_sym *prev, *next;
	kmod_sym_queue *queue;

	// module's list
	kmod_sym *kmod_prev, *kmod_next;
	kmod_sym_queue *kmod_queue;
};

struct kmod_sym_queue
{
	kmod_sym *head, *tail;
};

int kmod_sym_hash(char *name);

#define KMOD_SYM_MAX_HASH 64
extern kmod_sym_queue kmod_syms[KMOD_SYM_MAX_HASH];
extern mutex kmod_syms_mutex;

#define KMOD_NAME_MAX 32

struct kmod
{
	void *arch_info;

	kmod_sym_queue syms;

	int refcount;
	char name[KMOD_NAME_MAX];

	kmod *prev, *next;
	kmod_queue *queue;
};

struct kmod_queue
{
	kmod *head, *tail;
};

extern kmod_queue kmods;
extern mutex kmods_mutex;

void kmod_init();

kmod *kmod_load(void *image, char *name);
int   kmod_unload(kmod *mod);

typedef struct kernel_export
{
	int value;
	char *name;
} kernel_export;

void kmod_add_syms(kernel_export *exports, int count, kmod *mod);

// implemented in arch-specific code
void *kmod_arch_load(void *image, kmod_sym_queue *syms, void **initfunc);
int   kmod_arch_free(void *arch_info);

void kmod_arch_boot_modules();

#endif
