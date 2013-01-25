/*
 * kernel/kmod.c
 */

#include <kernel/kmod.h>
#include <kernel/exports.h>
#include <kernel/malloc.h>
#include <kernel/queue.h>
#include <kernel/arch/string.h>

mutex kmod_mutex;

kmod_queue kmods;
kmod_sym_queue kmod_syms[KMOD_SYM_MAX_HASH];

void kmod_init()
{
	mutex_init(&kmod_mutex);

	kmod_add_syms(kernel_exports,kernel_export_count,NULL);
}

void kmod_add_syms(kernel_export *exports,int count, kmod *mod)
{
	int i;

	// add exported kernel symbols
	for(i=0;i<count;i++)
	{
		kmod_sym *s;

		s = malloc(sizeof(kmod_sym));

		// fill in symbol info
		strncpy(s->name,exports[i].name,KMOD_SYM_NAME_MAX);
		s->value = exports[i].value;
		s->mod = mod;

		// insert into symbol hash
		queue_insert(&kmod_syms[kmod_sym_hash(s->name)],s);
	}
}

int kmod_sym_hash(char *name)
{
	int hash = 0x32f8d8cb;
	char c;

	while((c = *name++) != 0)
	{
		hash = (hash * 3) ^ (hash >> 16) ^ c;
	}

	return hash & (KMOD_SYM_MAX_HASH - 1);
}

kmod *kmod_load(void *image, char *name)
{
	kmod *mod;
	kmod_sym_queue syms;
	kmod_sym *sym;
	void *arch;
	void (*initfunc)();

	mutex_lock(&kmod_mutex);

	// first call the arch-specific code to do the actual loading
	arch = kmod_arch_load(image, &syms, (void **)&initfunc);

	if(!arch)
	{
		mutex_unlock(&kmod_mutex);
		return NULL;
	}

	// alloc the module structure
	mod = malloc(sizeof(kmod));

	// symbol list
	memcpy(&mod->syms,&syms,sizeof(kmod_sym_queue));

	// set the kmod pointer in all syms, and put all syms in the symbol hash
	for(sym=mod->syms.head;sym;sym=sym->kmod_next)
	{
		sym->mod = mod;

		queue_insert(&kmod_syms[kmod_sym_hash(sym->name)],sym);
	}

	// attributes
	mod->refcount = 0;
	strncpy(mod->name,name,KMOD_NAME_MAX);
	mod->arch_info = arch;

	// put it in the list
	queue_insert(&kmods,mod);

	// done
	mutex_unlock(&kmod_mutex);

	// now call the init function
	if(initfunc)
		initfunc();

	return mod;
}

int kmod_unload(kmod *mod)
{
	return -1;
}
