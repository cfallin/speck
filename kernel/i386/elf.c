/*
 * kenrel/i386/elf.c
 */

#include <kernel/i386/elf.h>
#include <kernel/kmod.h>
#include <kernel/i386/string.h>
#include <kernel/malloc.h>
#include <kernel/mutex.h>
#include <kernel/queue.h>
#include <kernel/i386/multiboot.h>
#include <kernel/i386/vid.h>

void kmod_arch_boot_modules()
{
	struct mod_list *mod;
	int i;

	if(!mboot_valid || !mboot_info->mods_count)
	{
		vid_puts("No boot modules.\n");
		return;
	}

	// get the module list array
	mod = (struct mod_list *)mboot_info->mods_addr;

	for(i=0;i<mboot_info->mods_count;i++)
	{
		char *cmdline, *p;

		// first extract the module name
		cmdline = (char *)mod[i].cmdline;

		// get the basename
		p = cmdline + strlen(cmdline);
		for(;p >= cmdline;p--)
			if(*p == '/')
				break;
		cmdline = p + 1;

		vid_puts("kmod: loading ");
		vid_puts(cmdline);
		vid_putc('\n');

		// if it doesn't end in ".o" skip it
		if(strcmp((cmdline+strlen(cmdline)-2),".o"))
			continue;

		// now load it
		if(kmod_load((void *)mod[i].mod_start,cmdline) == NULL)
		{
			vid_puts("kmod: module ");
			vid_puts(cmdline);
			vid_puts(" failed to load\n");
		}
		else
		{
			vid_puts("kmod: loaded ");
			vid_puts(cmdline);
			vid_putc('\n');
		}
	}
}

void *kmod_arch_load(void *image, kmod_sym_queue *syms, void **initfunc)
{
	elf_image *arch_info;
	elf_section *sctn, *lastsctn;
	kmod_sym *kmodsym;

	Elf32_Ehdr *header;
	Elf32_Shdr *section;
	Elf32_Sym *symtab;
	int symtab_count = 0;
	char *strtab = NULL;
	char *section_strtab;

	int i;

	// first validate the image
	header = (Elf32_Ehdr *)image;

	// magic number
	if(strncmp(&header->e_ident[EI_MAG0],ELF_MAGIC,4))
		return NULL;

	if(header->e_ident[EI_CLASS] != ELFCLASS32)
		return NULL;

	if(header->e_ident[EI_DATA] != ELFDATA2LSB) // little endian
		return NULL;

	if(header->e_ident[EI_VERSION] != 1)
		return NULL;

	if(header->e_type != ET_REL)
		return NULL;

	if(header->e_machine != EM_386)
		return NULL;

	// image looks good; now look through the sections
	section = (Elf32_Shdr *) (((char *)image) + header->e_shoff);

	// get the section name string table
	section_strtab = ((char *)image) + section[header->e_shstrndx].sh_offset;

	// find the string table for symbol names - first string table that's not
	// header->e_shstrndx
	strtab = NULL;
	for(i=0;i<header->e_shnum;i++)
	{
		if(section[i].sh_type == SHT_STRTAB && i != header->e_shstrndx)
		{
			strtab = ((char *)image) + section[i].sh_offset;
			break;
		}
	}

	// we should have a string table; if not, error
	if(!strtab)
		return NULL;

	// find the symbol table
	symtab = NULL;
	for(i=0;i<header->e_shnum;i++)
	{
		if(section[i].sh_type == SHT_SYMTAB)
		{
			symtab = (Elf32_Sym *) (((char *)image) + section[i].sh_offset);
			symtab_count = section[i].sh_size / sizeof(Elf32_Sym);
			break;
		}
	}

	// we have to have a symbol table too; error if not
	if(!symtab)
		return NULL;

	// now make sure all the external references are satisfied
	// also check for "common" section symbols
	for(i=1;i<symtab_count;i++)
	{
		char *symname;
		int hash;
		kmod_sym *s;

		if(symtab[i].st_shndx == SHN_COMMON)
			return NULL;

		if(symtab[i].st_shndx != SHN_UNDEF)
			continue;

		symname = &strtab[symtab[i].st_name];
		hash = kmod_sym_hash(symname);

		for(s=kmod_syms[hash].head;s;s=s->next)
			if(!strcmp(s->name,symname))
				break;

		if(!s)
		{
			vid_puts("ELF link: undefined reference: ");
			vid_puts(symname);
			vid_putc('\n');
			return NULL;
		}

		// store symbol struct address in symbol value
		symtab[i].st_value = (Elf32_Addr)s;
	}

	// make sure there are no global symbol conflicts
	for(i=1;i<symtab_count;i++)
	{
		int hash;

		if(ELF32_ST_BIND(symtab[i].st_info) != STB_GLOBAL)
			continue;

		if(symtab[i].st_shndx == SHN_UNDEF)
			continue;

		hash = kmod_sym_hash(strtab + symtab[i].st_name);

		for(kmodsym=kmod_syms[hash].head;kmodsym;kmodsym=kmodsym->next)
		{
			if(!strcmp(kmodsym->name,strtab+symtab[i].st_name))
			{
				vid_puts("ELF link: global symbol conflict: ");
				vid_puts(kmodsym->name);
				vid_putc('\n');
				return NULL;
			}
		}
	}

	// now we know that the image will load without errors; go ahead and
	// commit an elf_image struct

	arch_info = malloc(sizeof(elf_image));
	arch_info->image = image;

	// now create the section list
	lastsctn = NULL;
	for(i=1;i<header->e_shnum;i++)
	{
		// we only want sections that are used by the program (ie, .text,
		// .data and .bss)
		if( ! (section[i].sh_flags & SHF_ALLOC) )
		{
			// set sh_addr
			section[i].sh_addr = (Elf32_Addr)(((char *)image ) +
				section[i].sh_offset);
			continue;
		}

		sctn = malloc(sizeof(elf_section));
		if(!lastsctn)
			arch_info->sections = sctn;

		// fill in the info
		sctn->size = section[i].sh_size;
		sctn->index = i;
		strncpy(sctn->name,&section_strtab[section[i].sh_name],
			ELF_SECTION_NAME_MAX);

		sctn->in_image = (section[i].sh_type == SHT_PROGBITS) ? 1 : 0;

		// if the section doesn't exist in the image (ie, .bss) then allocate
		// some memory for it
		if(!sctn->in_image)
		{
			sctn->ptr = malloc(sctn->size);
			memset(sctn->ptr,0,sctn->size);
		}
		else
			// otherwise just set the pointer
			sctn->ptr = ((char *)image) + section[i].sh_offset;

		// update the address in the image
		section[i].sh_addr = (Elf32_Addr)sctn->ptr;

		// link it into the chain
		sctn->next = NULL;
		if(lastsctn)
			lastsctn->next = sctn;
	}

	// now calculate symbol addresses, handling external symbol refcounts
	for(i=1;i<symtab_count;i++)
	{
		kmod_sym *s;

		// external references
		if(symtab[i].st_shndx == SHN_UNDEF)
		{
			// first get the symbol struct
			s = (kmod_sym *)symtab[i].st_value;

			// increment the module's refcount
			if(s->mod)
				s->mod->refcount++;

			// now store the value
			symtab[i].st_value = s->value;
		}
		// absolute value - don't add section base
		else if(symtab[i].st_shndx == SHN_ABS)
			continue;
		// "normal" symbol - add section base
		else
			symtab[i].st_value += section[symtab[i].st_shndx].sh_addr;
	}

	// now do relocations
	for(i=1;i<header->e_shnum;i++)
	{
		Elf32_Rel *rel;
		Elf32_Rela *rela;
		int rel_count;
		int j;
		char *base = NULL;

		// figure out the base of the section we're relocating
		if(section[i].sh_type == SHT_REL || section[i].sh_type == SHT_RELA)
			base = (char *)section[section[i].sh_info].sh_addr;
		else
			// skip non-rel/rela sections
			continue;

		// relocations without addends
		if(section[i].sh_type == SHT_REL)
		{
			// get the array
			rel = (Elf32_Rel *)section[i].sh_addr;
			rel_count = section[i].sh_size / sizeof(Elf32_Rel);

			for(j=0;j<rel_count;j++)
			{
				int *r = (int *)(base + rel[j].r_offset);

				switch(ELF32_R_TYPE(rel[j].r_info))
				{
				case R_386_32:
					*r += symtab[ELF32_R_SYM(rel[j].r_info)].st_value;
					break;
				case R_386_PC32:
					*r += symtab[ELF32_R_SYM(rel[j].r_info)].st_value - (int)r;
					break;
				}
			}
		}
		// relocations with addends
		else if(section[i].sh_type == SHT_RELA)
		{
			// get the array
			rela = (Elf32_Rela *)section[i].sh_addr;
			rel_count = section[i].sh_size / sizeof(Elf32_Rela);

			for(j=0;j<rel_count;j++)
			{
				int *r = (int *)(base + rela[j].r_offset);
				switch(ELF32_R_TYPE(rela[j].r_info))
				{
				case R_386_32:
					*r = symtab[ELF32_R_SYM(rela[j].r_info)].st_value + 
						rela[j].r_addend;
					break;
				case R_386_PC32:
					*r = symtab[ELF32_R_SYM(rela[j].r_info)].st_value +
						rela[j].r_addend - (int)r;
					break;
				}
			}
		}
	}

	*initfunc = NULL;

	// now create the list of kmod-exported symbols
	memset(syms,0,sizeof(kmod_sym_queue));
	for(i=1;i<symtab_count;i++)
	{
		if(ELF32_ST_BIND(symtab[i].st_info) != STB_GLOBAL)
			continue;

		// handle the function "kmod_init" specially
		if(!strcmp(strtab + symtab[i].st_name, "kmod_init"))
		{
			*initfunc = (void *)symtab[i].st_value;
			continue;
		}

		kmodsym = malloc(sizeof(kmod_sym));

		strncpy(kmodsym->name,strtab + symtab[i].st_name,KMOD_SYM_NAME_MAX);
		kmodsym->value = symtab[i].st_value;

		// put it on the kmod queue
		queue_insert_generic(syms,kmodsym,kmod_);
	}

	return arch_info;
}

int kmod_arch_free(void *arch_info)
{
	return -1;
}
